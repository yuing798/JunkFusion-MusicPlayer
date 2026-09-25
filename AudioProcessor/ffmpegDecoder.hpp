#pragma once
#include "./AudioRingBuffer.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <zmq.hpp>
extern "C" {
#include <libavutil/channel_layout.h>
}
#include <optional>
#include <string>

class FFmpegDecoder : public juce::Thread {
private:
    AVChannelLayout targetChannelLayout; // 输出通道布局
    double targetSampleRate{44100.0};    // 输出采样率
    std::string path;                    // 当前正在播放的歌曲路径
    AudioRingBuffer* mRingBuffer;
    double currentTimeStamp{0.0};           // 当前播放到了哪里
    std::atomic<bool> isRequestSeek{false}; // 当前循环是否请求搜索
    std::atomic<double> targetSeconds{0.0}; // 请求搜索的目标位置
    bool isNatureComplete{true};            // 歌曲是自然完成还是强制结束的

public:
    void prepareToPlay(juce::AudioChannelSet, double); // 这个是在改变全局播放设置的时候调用
    void run() override;
    void play(std::string songPath, double targetSeconds);
    explicit FFmpegDecoder(AudioRingBuffer* ringBuffer);
    ~FFmpegDecoder();

    std::function<void(std::string)> sendErrorMsg; // 发送错误信息
    std::function<void(void)> onNatureComplete;    // 歌曲自然完成(直接播放下一首歌)
};