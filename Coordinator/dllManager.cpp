#include "./dllManager.hpp"
#include "./SpectrumController.hpp"
#include "CenterDbManage/dbContext.hpp"
#include "SongsInsertHelper/FolderCheckTimer.hpp"
#include "SongsInsertHelper/SongImport.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "juce_core/juce_core.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

std::unique_ptr<dllManager> dllManager::instance = nullptr;

dllManager::dllManager() {}

void dllManager::init(const char* cacheDirId, const char* exeDirPtr) {

    cacheDir = juce::File{cacheDirId};
    logInfoDir = cacheDir.getChildFile("log");
    if (!logInfoDir.exists()) logInfoDir.createDirectory();
    imageDir = cacheDir.getChildFile("image");
    if (!imageDir.exists()) imageDir.createDirectory();
    // 单独导入的歌曲拷贝目录，和 JunkFusion.db 在同一个缓存文件夹下
    songsDir = cacheDir.getChildFile("songs");
    if (!songsDir.exists()) songsDir.createDirectory();

    constexpr size_t kUiMaxSize = 5 * 1024 * 1024; // 5 MB
    constexpr size_t kUiMaxFiles = 3;

    {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logInfoDir.getChildFile("dll.log").getFullPathName().toStdString(),
            kUiMaxSize,
            kUiMaxFiles
        );
        // 全局日志格式：时间戳 + 级别 + 线程ID + 消息体
        const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";
        auto dllLogger = std::make_shared<spdlog::logger>(LogDllID, std::move(sink));
        dllLogger->set_pattern(pattern);
#ifdef JF_DEBUG
        dllLogger->set_level(spdlog::level::debug);
        dllLogger->flush_on(spdlog::level::debug);
#else
        dllLogger->set_level(spdlog::level::info);
        dllLogger->flush_on(spdlog::level::err);
#endif

        spdlog::register_logger(dllLogger);
    }

    auto logger{spdlog::get(LogDllID)};
    logger->debug("准备初始化音频进程");

    mAudioProcessCoordinator = std::make_unique<AudioProcessCoordinator>();
    mOscSender = std::make_unique<OscSender>();

    // ── 崩溃自愈：看门狗把音频进程重新拉起来之后要做的收尾 ──
    //
    // 插件能把音频进程直接带走（见 junk_fusion_plugin_lab/docs/host-integration.md），
    // 进程内救不了，只能由 AudioProcessCoordinator 的看门狗发现并重启。
    // 重启后有两件事必须做：
    //   1. 把频谱共享内存的名字重新下发一遍（新进程不知道这块内存存在）；
    //   2. 通知前端：播放已经中断，别让 UI 还显示在播。
    mAudioProcessCoordinator->onAudioProcessRestarted = [this](int attempt) {
        auto restartLogger{spdlog::get(LogDllID)};

        if (attempt < 0) {
            // 看门狗放弃了（重启太频繁）：交给前端弹致命提示，这里只记日志
            if (restartLogger) restartLogger->critical("音频进程反复崩溃，已停止自动重启");
            if (onAudioProcessRestarted) onAudioProcessRestarted(-1);
            return;
        }

        if (restartLogger) restartLogger->warn("音频进程已重启（第 {} 次）", attempt);

        if (mSpectrumController != nullptr) mSpectrumController->reannounce();
        // 链**不重发**：刚死过一次的插件自动恢复就是崩溃循环（host-integration §6.2）
        if (mPluginChainController != nullptr) mPluginChainController->resetAfterRestart();
        if (onAudioProcessRestarted) onAudioProcessRestarted(attempt);
    };

    if (!mAudioProcessCoordinator->start(
            juce::File{exeDirPtr}.getChildFile("JunkFusionAudioProcess.exe"),
            mOscSender->getPort(),
            juce::File{cacheDirId}
        )) {
        logger->critical("音频进程启动或握手失败");
    }
    logger->debug("已通知音频进程启动");

    // ── 频谱：共享内存在这里创建，然后把名字下发给音频进程 ──
    // 数据面是两端直连共享内存；Coordinator 只负责递名片 + 给前端读快照。
    mSpectrumController = std::make_unique<SpectrumController>();
    mSpectrumController->init(
        [this](const std::string& message) { sendMessage2AudioProcess(message); }
    );

    // ── 全局效果器链（8 槽）──
    // 权威副本在这里，音频进程只是"执行 + 回报状态"
    mPluginChainController = std::make_unique<PluginChainController>();
    mPluginChainController->init(
        [this](const std::string& message) { sendMessage2AudioProcess(message); }
    );
    // 预检用的扫描子进程：和批量扫描同一个 exe，只是走 --preflight 模式
    mPluginChainController->setScanExe(
        juce::File{exeDirPtr}.getChildFile("JunkFusionPluginScan.exe")
    );

    juce::File dbFile{cacheDir.getChildFile("JunkFusion.db")};

    // 用户有时只删主库、留下 -wal/-shm。WAL 模式下 SQLite 会拿这份旧日志去
    // 恢复一个**全新建出来的空库**，结果就是"半损坏"状态（表现为
    // `out of memory` / 找不到歌曲 / 查询返回空）。主库不存在时把残留日志清掉，
    // 让数据库干干净净地重建。
    if (!dbFile.existsAsFile()) {
        for (const char* suffix : {"-wal", "-shm"}) {
            const juce::File stale{dbFile.getFullPathName() + suffix};
            if (!stale.existsAsFile()) continue;
            stale.deleteFile();
            logger->warn(
                "检测到残留数据库日志，已清理:{}{}",
                dbFile.getFileName().toStdString(),
                suffix
            );
        }
    }

    mDbContext = std::make_unique<DbContext>(dbFile.getFullPathName().toStdString());

    mSongImport = std::make_unique<SongImport>(*(mDbContext.get()), imageDir, songsDir);

    // ── 插件库（扫描结果）和扫描任务 ──
    // 扫描任务必须在 dbContext 之后创建、在它之前销毁（析构里显式 stopAndWait）
    mPluginCatalog = std::make_unique<PluginHost::PluginCatalogManage>(mDbContext->dbManage);
    mPluginCatalog->ensureDefaultDirs();
    mPluginScanJob = std::make_unique<PluginHost::PluginScanJob>(
        *mPluginCatalog,
        juce::File{exeDirPtr}.getChildFile("JunkFusionPluginScan.exe"),
        cacheDir.getChildFile("pluginScan")
    );
    logger->debug(
        "插件库已就绪，扫描进程:{}",
        juce::File{exeDirPtr}.getChildFile("JunkFusionPluginScan.exe").getFullPathName().toStdString()
    );

    // 文件夹定时监控（独立线程，一分钟一轮）。必须在 mSongImport / mDbContext
    // 之后创建、在它们之前销毁（见析构函数）
    mFolderCheckTimer =
        std::make_unique<FolderCheckTimer>(*(mDbContext.get()), *(mSongImport.get()));
    mFolderCheckTimer->start();
}

void dllManager::sendMessage2AudioProcess(const std::string& obj) {
    mAudioProcessCoordinator->mAudioProcessPusher->sendMessage(obj);
}

void dllManager::notifySongUpdated(int64_t songId) {
    if (!mDbContext || !mSongImport || !mSongImport->onAddOrUpdateSong) return;

    try {
        const auto info = mDbContext->songsManage.getBasicSongInfo(songId);
        mSongImport->onAddOrUpdateSong(ConvertUtils::object2Uint8t(info.toBasicJson()));
    } catch (const std::exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("dllManager::notifySongUpdated 失败 songId={}:{}", songId, e.what());
    }
}

dllManager::~dllManager() {
    // 先停掉监控线程：它还持有 SongImport / DbContext 的引用，
    // 必须在这两个析构之前 join 掉
    if (mFolderCheckTimer) {
        mFolderCheckTimer->stop();
        mFolderCheckTimer.reset();
    }

    // 扫描线程持有 PluginCatalogManage→CenterDbManage 的引用，同理先停
    if (mPluginScanJob) {
        mPluginScanJob->stopAndWait();
        mPluginScanJob.reset();
    }
    mPluginCatalog.reset();

    // 音频进程的看门狗 / 收发线程都会打日志，必须在 spdlog::shutdown() **之前**
    // 全部停掉。顺序反了的话，线程会在 registry 销毁的同时调 spdlog，
    // 表现为"只在进程退出时崩在 spdlogd.dll（0xC0000005）"，实测踩过。
    mAudioProcessCoordinator.reset();

    spdlog::get(LogDllID)->debug("---------------------------------------");
    spdlog::shutdown();
}

Yvar dllManager::openNewSongFolder(const juce::File& path) {
    auto& songFolderManage = getDbContext()->songFolderManage;

    return songFolderManage.getChildDirAndSongIdList(path);
}
