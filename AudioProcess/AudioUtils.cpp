#include "./AudioUtils.hpp"
#include "AudioDefs.hpp"
#include "constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
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

    // 初始化 spdlog 的全局静态异步线程池，而不是使用局部变量
    spdlog::init_thread_pool(8192, 1);

    {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile,
            kAudioMaxSize,
            kAudioMaxFiles
        );
        audioLogger = std::make_shared<spdlog::async_logger>(
            LogAudioID,
            std::move(sink),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
        audioLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
#ifdef JF_DEBUG
        audioLogger->set_level(spdlog::level::debug);
#else
        audioLogger->set_level(spdlog::level::err);
#endif
        audioLogger->flush_on(spdlog::level::err); // 遇到错误立刻刷盘
        spdlog::register_logger(audioLogger);
    }
    spdlog::get(LogAudioID)->debug("音频进程日志初始化完成");
}

juce::AudioBuffer<float> AudioUtils::generateSinTable(int num4pi) {
    int tableSize{1024};
    juce::AudioBuffer<float> tableBuffer;
    tableBuffer.clear();
    tableBuffer.setSize(1, tableSize);

    for (int index = 0; index < tableSize; index++) {

        float phase =
            (static_cast<float>(index) / tableSize) * num4pi * juce::MathConstants<float>::pi;
        tableBuffer.getWritePointer(0)[index] = std::sin(phase);
    }
    return tableBuffer;
}

juce::AudioBuffer<float> AudioUtils::generateCosTable(int num4pi) {
    int tableSize{1024};
    juce::AudioBuffer<float> tableBuffer;
    tableBuffer.clear();
    tableBuffer.setSize(1, tableSize);

    for (int index = 0; index < tableSize; index++) {

        float phase =
            (static_cast<float>(index) / tableSize) * num4pi * juce::MathConstants<float>::pi;
        tableBuffer.getWritePointer(0)[index] = std::cos(phase);
    }
    return tableBuffer;
}