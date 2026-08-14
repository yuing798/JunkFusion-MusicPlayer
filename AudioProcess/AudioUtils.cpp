#include "./AudioUtils.hpp"
#include "AudioDefs.hpp"
#include "juce_core/juce_core.h"
#include <memory>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

void AudioUtils::initAudioLogger(juce::File cacheDir) {
    std::shared_ptr<spdlog::logger> audioLogger;
    constexpr size_t kAudioMaxSize = 5 * 1024 * 1024; // 5 MB
    constexpr size_t kAudioMaxFiles = 3;
    auto logDir{cacheDir.getChildFile("log")};
    if (!logDir.exists()) logDir.createDirectory();
    auto logFile{logDir.getChildFile("audioProcess.log").getFullPathName().toStdString()};

    auto audioThreadPool = std::make_shared<spdlog::details::thread_pool>(8192, 1);

    {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile,
            kAudioMaxSize,
            kAudioMaxFiles
        );
        audioLogger = std::make_shared<spdlog::async_logger>(
            AudioDefs::LogAudioId,
            std::move(sink),
            audioThreadPool,
            spdlog::async_overflow_policy::block
        );
        audioLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
#ifdef JF_DEBUG
        audioLogger->set_level(spdlog::level::debug);
#else
        audioLogger->set_level(spdlog::level::err);
#endif
        spdlog::register_logger(audioLogger);
    }
}