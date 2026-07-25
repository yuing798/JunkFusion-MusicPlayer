#pragma once
#include "./AudioRingBuffer.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "model.h"
#include <cstdint>
#include <string>

class FFmpegDecoder : public juce::Thread {
private:
    int numChannels{2};
    double sampleRate{defaultSampleRate};
    int64_t currentSongId{0};
    playInfo mPlayInfo;
    AudioRingBuffer& ringBuffer;

public:
    void prepareToPlay(int, double); // 这个是在改变全局播放设置的时候调用
    void run() override;
    void prepareToPlayNewSong(int64_t songId); // 这个是在播放新歌的时候调用
    FFmpegDecoder(AudioRingBuffer&);
    ~FFmpegDecoder();
};