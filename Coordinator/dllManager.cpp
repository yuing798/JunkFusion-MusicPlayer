#include "./dllManager.hpp"
#include "Macro/coordinatorMacro.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

std::unique_ptr<dllManager> dllManager::instance = nullptr;

dllManager::dllManager() {}

void dllManager::init(const char* cacheDirId, const char* exeDirPtr) {

    cacheDir = juce::File{cacheDirId};
    logInfoDir = cacheDir.getChildFile("log");
    if (!logInfoDir.exists()) logInfoDir.createDirectory();
    songImageDir = cacheDir.getChildFile("songImage");
    if (!songImageDir.exists()) songImageDir.createDirectory();

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
#else
        dllLogger->set_level(spdlog::level::info);
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
    // if(!dbFile.existsAsFile()) dbFile.cr  不用，因为SQLite::OPEN_CREATE会初始化文件
    db = std::make_unique<SQLite::Database>(
        dbFile.getFullPathName().toStdString(),
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
    db->exec("PRAGMA journal_mode=WAL;");  // 写操作并发友好
    db->exec("PRAGMA busy_timeout=5000;"); // 遇到锁最多等 5 秒，不立即报错

    // 把 db 指针或引用传给各表
    songs = std::make_unique<SongsManage>(*db);
}

void dllManager::sendMessage2AudioProcess(juce::var& obj) {
    auto jsonStr{juce::JSON::toString(obj).toStdString()};
    mAudioProcessCoordinator->mAudioProcessPusher->sendMessage(jsonStr);
}

dllManager::~dllManager() {
    spdlog::get(LogDllID)->debug("---------------------------------------");
    spdlog::shutdown();
}
