#pragma once
#include "constants.h"
#include <cstdint>
#include <string>
struct playInfo {
    double duration{0.0};
    std::string path;
    int codecId{0};
    double originalSampleRate{defaultSampleRate};
    int64_t bitRate{0};                    // 比特率（kbps）
    int originalNumChannels{0};            // 通道数
    uint64_t originalChannelLayoutMask{0}; // 原始通道布局掩码
    int bitDepth{0};                       // 位深
};