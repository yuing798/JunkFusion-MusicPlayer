#include "./dllManager.hpp"
#include "CenterDbManage/dbContext.hpp"
#include "SongsInsertHelper/SongImport.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include "juce_core/juce_core.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <variant>

std::unique_ptr<dllManager> dllManager::instance = nullptr;

dllManager::dllManager() {}

void dllManager::init(const char* cacheDirId, const char* exeDirPtr) {

    cacheDir = juce::File{cacheDirId};
    logInfoDir = cacheDir.getChildFile("log");
    if (!logInfoDir.exists()) logInfoDir.createDirectory();
    imageDir = cacheDir.getChildFile("image");
    if (!imageDir.exists()) imageDir.createDirectory();

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

    juce::File dbFile{cacheDir.getChildFile("JunkFusion.db")};

    mDbContext = std::make_unique<DbContext>(dbFile.getFullPathName().toStdString());

    mSongImport = std::make_unique<SongImport>(*(mDbContext.get()), imageDir);
}

void dllManager::sendMessage2AudioProcess(const std::string& obj) {
    mAudioProcessCoordinator->mAudioProcessPusher->sendMessage(obj);
}

dllManager::~dllManager() {
    spdlog::get(LogDllID)->debug("---------------------------------------");
    spdlog::shutdown();
}

Yvar dllManager::openNewSongFolder(const juce::File& path) {
    auto& songFolderManage = getDbContext()->songFolderManage;
    auto dirsNeedToRescan = songFolderManage.checkDirUpdateRecursion(path);
    for (auto& dirNeedToRescan : dirsNeedToRescan) {
        auto children = dirNeedToRescan.findChildFiles(juce::File::findFiles, false);
        for (auto& child : children) {
            mSongImport->checkAndInsertSong(child, false);
        }
    }
    return songFolderManage.getChildDirAndSongIdList(path);
}
