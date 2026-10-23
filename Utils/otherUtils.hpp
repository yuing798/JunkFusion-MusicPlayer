#pragma once
#include "juce_core/juce_core.h"
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>

namespace OtherUtils {

    void initAudioLogger(juce::File cacheDir); // 初始化日志
    // 输出错误码
    std::string ffmpegErrorOutput(int result);

    void writeEmergencyLog(std::string message);

} // namespace OtherUtils