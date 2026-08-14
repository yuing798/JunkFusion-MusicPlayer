#pragma once
#include "./AudioRingBuffer.hpp"
#include "constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <zmq.hpp>
extern "C" {
#include <libavutil/channel_layout.h>
}
#include <optional>
#include <string>

class FFmpegDecoder : public juce::Thread {
private:
    AVChannelLayout targetChannelLayout;        // 输出通道布局
    double targetSampleRate{defaultSampleRate}; // 输出采样率
    std::string path;                           // 当前正在播放的歌曲路径
    AudioRingBuffer& ringBuffer;
    double currentTimeStamp{0.0}; // 当前播放到了哪里
    bool playState{false};        // false为暂停,true为播放
    zmq::socket_t& mSocket;

public:
    void prepareToPlay(juce::AudioChannelSet, double); // 这个是在改变全局播放设置的时候调用
    void run() override;
    void setNewPlayState(std::string songPath); // 要播放新的歌曲了
    explicit FFmpegDecoder(AudioRingBuffer&, zmq::socket_t&);
    ~FFmpegDecoder();
};