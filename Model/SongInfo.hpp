#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <optional>
#include <vector>

struct SongInfo {
    int64_t songId{0};

    double duration{0.0}; // 歌曲时长（秒）时长应该每条流都一样

    // ── 3. 标签信息 ──
    juce::String title; // 名称，因为有的音乐文件有自带的标题(不是文件名!!!)
    // 这个标题不需要std::optional保底，因为我在FFmpeg提取元数据函数中已经写好了如果没有title直接把文件stem名称赋值给title

    std::optional<juce::StringArray> artists; // 艺术家名称
    std::optional<juce::String> album;        // 专辑
    std::optional<int> trackNumber;           // 轨道号（std::nullopt 表示不存在）
    std::optional<int> discNumber;
    // 这些基础数据都是文件容器层面的

    // ── 2. FFmpeg 解码层信息 ──

    int64_t bitRate = 0; // 比特率（kbps）
    int sampleRate{0};   // 采样率（Hz）ffmpeg只能读取整数采样率，
    // 实际上也基本都是整数采样率，processBlock中用double采样率是为了计算精度平衡
    juce::String channelLayout;            // 通道布局(通道布局是一定有的)
    int bitDepth = 0;                      // 位深
    std::optional<juce::String> codecName; // 编码器名称//因为编码器名称不一定有，但是编码器ID一定有

    // ── 4. AI 分析信息 ──
    std::optional<int> bpm;          // 节拍数（std::nullopt 表示未知）
    std::optional<juce::String> key; // 调性（如 C major, A minor）

    // 5.用户信息
    bool isMyLike{0}; // 是否添加到了我喜欢列表
    int playNum{0};   // 已经播放了多少次

    std::optional<juce::String> hash;
    std::vector<double> timeDomainSpec;

    // 将songInfo转化为var，才能推送给前端
    juce::var toCompleteJson();
    juce::var toBasicJson();
};
