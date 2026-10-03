#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include <memory>

// 基础控制滑块组，包括，音量，倍速，音高偏移
class BaseControlSlider {
private:
    struct Impl;
    std::unique_ptr<Impl> mImpl{std::make_unique<Impl>()};

public:
    void prepareToPlay(double samplerate, int numChannels);
    void processBlock(juce::AudioBuffer<float>& buffer);
};