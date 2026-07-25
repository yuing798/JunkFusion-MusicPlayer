#pragma once
#include "./AudioRingBuffer.hpp"
#include "juce_core/juce_core.h"
#include "model.h"
#include <cstdint>
#include <string>

class FFmpegDecoder : public juce::Thread {
private:
    int64_t currentSongId{0};
    playInfo mPlayInfo;
    AudioRingBuffer& ringBuffer;

public:
    void prepareToPlay(int, double);
    void run() override;
    void prepareToPlayNewSong(int64_t songId);
    FFmpegDecoder(AudioRingBuffer&);
    ~FFmpegDecoder();
};