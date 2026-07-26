#pragma once
#include "./AudioRingBuffer.hpp"
#include "constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "model.h"
#include <cstdint>
extern "C" {
#include <libavutil/channel_layout.h>
}
#include <optional>
#include <string>

class FFmpegDecoder : public juce::Thread {
private:
    AVChannelLayout outputChannelLayout;
    double sampleRate{defaultSampleRate}; // 输出采样率
    int64_t currentSongId{0};
    playInfo mPlayInfo; // 原始音频的播放信息
    AudioRingBuffer& ringBuffer;

public:
    void prepareToPlay(juce::AudioChannelSet, double); // 这个是在改变全局播放设置的时候调用
    void run() override;
    void prepareToPlayNewSong(int64_t songId); // 这个是在播放新歌的时候调用
    FFmpegDecoder(AudioRingBuffer&);
    ~FFmpegDecoder();
};