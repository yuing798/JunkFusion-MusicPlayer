#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <memory>
#include <spdlog/logger.h>

namespace AudioUtils {
    void initAudioLogger(juce::File cacheDir); // 初始化日志

    // 生成正弦表
    //  num4pi:需要从0 ~ num4pi * pi的区域的数组
    juce::AudioBuffer<float> generateSinTable(int num4pi);

    // 生成余弦表
    //  num4pi:需要从0 ~ num4pi * pi的区域的数组
    juce::AudioBuffer<float> generateCosTable(int num4pi);
} // namespace AudioUtils