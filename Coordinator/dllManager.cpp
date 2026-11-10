#include "./dllManager.hpp"
#include "./EqController.hpp"
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
    if (!mAudioProcessCoordinator->start(
            juce::File{exeDirPtr}.getChildFile("JunkFusionAudioProcess.exe"),
            mOscSender->getPort(),
            juce::File{cacheDirId}
        )) {
        logger->critical("音频进程启动或握手失败");
    }
    logger->debug("已通知音频进程启动");

    // ── EQ 的下发通道 ──
    // 拖拽中的连续量走 OSC/UDP（高频、允许丢帧），结构变更/最终值走 ZMQ JSON（必达）。
    // 两条通道都通过 ParamLink 统一节流并自动补 rev，音频进程据此丢弃迟到的旧帧。
    mEqController = std::make_unique<EqController>();
    mEqController->init(
        [this](const char* address, const float* args, int numArgs, uint32_t rev) {
            if (mOscSender == nullptr) return;
            try {
                juce::OSCMessage message{juce::String(address)};
                for (int i = 0; i < numArgs; ++i)
                    message.addFloat32(args[i]);
                message.addInt32(static_cast<juce::int32>(rev));
                mOscSender->sendMsg(message);
            } catch (const std::exception& e) {
                // 地址写错时 OSCAddress 会抛异常，这里兜住，不要让 UI 线程崩掉
                if (auto logger = spdlog::get(LogDllID))
                    logger->error("EQ 的 OSC 地址无效:{} ({})", address, e.what());
            }
        },
        [this](const Yvar& envelope) { sendMessage2AudioProcess(envelope.dump()); }
    );

    // ── 频谱：共享内存在这里创建，然后把名字下发给音频进程 ──
    // 数据面是两端直连共享内存；Coordinator 只负责递名片 + 给前端读快照。
    mSpectrumController = std::make_unique<SpectrumController>();
    mSpectrumController->init(
        [this](const std::string& message) { sendMessage2AudioProcess(message); }
    );

    juce::File dbFile{cacheDir.getChildFile("JunkFusion.db")};

    mDbContext = std::make_unique<DbContext>(dbFile.getFullPathName().toStdString());

    mSongImport = std::make_unique<SongImport>(*(mDbContext.get()), imageDir, songsDir);

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

    spdlog::get(LogDllID)->debug("---------------------------------------");
    spdlog::shutdown();
}

Yvar dllManager::openNewSongFolder(const juce::File& path) {
    auto& songFolderManage = getDbContext()->songFolderManage;

    return songFolderManage.getChildDirAndSongIdList(path);
}
