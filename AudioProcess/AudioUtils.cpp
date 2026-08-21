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

juce::AudioBuffer<float> AudioUtils::generateSinTable(double num4pi) {
    juce::AudioBuffer<float> tableBuffer;
    tableBuffer.clear();
    tableBuffer.setSize(1, AudioUtils::lookupTableSize);

    for (int index = 0; index < AudioUtils::lookupTableSize; index++) {

        float phase = (static_cast<float>(index) / AudioUtils::lookupTableSize) * num4pi *
                      juce::MathConstants<float>::pi;
        tableBuffer.getWritePointer(0)[index] = std::sin(phase);
    }
    return tableBuffer;
}

juce::AudioBuffer<float> AudioUtils::generateCosTable(double num4pi) {
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

float AudioUtils::getLinearInterpolator(const float* data, int size, float process) {
    int index1 = static_cast<int>(process * size);
    int index2 = getCircularBufferIndex(index1 + 1, size);
    float fraction = process - index1;

    return (1.0f - fraction) * data[index1] + fraction * data[index2];
}

float AudioUtils::getLagrangeInterpolator(const float* data, int size, float process) {
    // 原理：用一个 N 阶多项式穿过 N+1 个最近的样本点，然后取多项式在所需延迟位置的值。
    // 奇数阶的拉格朗日插值（如 3 阶，用 4 个样本）相当于一个对称的 FIR 滤波器，其系数是分数延迟 d
    // 的简单多项式。
    int intIndex = static_cast<int>(process * size);
    float distanceIndex2IntIndex = process * size - intIndex;
    float nagetive1 = data[getCircularBufferIndex(intIndex - 1, size)];
    float intSample = data[intIndex];
    float positive1 = data[getCircularBufferIndex(intIndex + 1, size)];
    float positive2 = data[getCircularBufferIndex(intIndex + 2, size)];

    float outputSample = 0.0f;
    float distanceIndex2IntIndexMinus1 = distanceIndex2IntIndex - 1;
    float distanceIndex2IntIndexMinus2 = distanceIndex2IntIndex - 2;
    float distanceIndex2IntIndexPlus1 = distanceIndex2IntIndex + 1;

    outputSample -= distanceIndex2IntIndex * distanceIndex2IntIndexMinus1 *
                    distanceIndex2IntIndexMinus2 * 0.16667f * nagetive1;
    outputSample += distanceIndex2IntIndexPlus1 * distanceIndex2IntIndexMinus1 *
                    distanceIndex2IntIndexMinus2 * 0.5f * intSample;
    outputSample -= distanceIndex2IntIndexPlus1 * distanceIndex2IntIndex *
                    distanceIndex2IntIndexMinus2 * 0.5f * positive1;
    outputSample += distanceIndex2IntIndexPlus1 * distanceIndex2IntIndex *
                    distanceIndex2IntIndexMinus1 * 0.16667f * positive2;
    return outputSample;
}