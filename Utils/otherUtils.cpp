#include "./otherUtils.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <spdlog/common.h>
#include <stdio.h>
#include <string>
#include <utility>
#include <zmq.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h> //负责日志信息
#include <libavutil/samplefmt.h>
}

void OtherUtils::initAudioLogger(juce::File cacheDir) {
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
        audioLogger->flush_on(spdlog::level::debug);
#else
        audioLogger->set_level(spdlog::level::info);
        audioLogger->flush_on(spdlog::level::err); // 遇到错误立刻刷盘
#endif

        spdlog::register_logger(audioLogger);
    }
    spdlog::get(LogAudioID)->debug("音频进程日志初始化完成");
}

std::string OtherUtils::ffmpegErrorOutput(int result) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    // 将错误码ret转换为可读字符串存入errbuf
    av_strerror(result, errbuf, sizeof(errbuf));
    return std::string(errbuf);
}

/**
 * @brief 紧急降级日志函数(绝不抛出异常，写完立刻刷盘）
 * @param message 待写入的日志消息字符串
 */
void OtherUtils::writeEmergencyLog(std::string message) {
#ifdef JF_DEBUG
    std::ofstream testFile(
        "D:/audio_develop/Junk-Fusion/text.txt",
        std::ios::out | std::ios::app
    ); // 追加模式
    if (testFile.is_open()) {
        testFile << message.c_str() << std::endl;
        testFile.flush();
        testFile.close();
    }
#endif
}
