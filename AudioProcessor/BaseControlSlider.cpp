#include "./BaseControlSlider.hpp"
#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"

struct BaseControlSlider::Impl {
    SmoothLinearValue mVolume;       // 音量控制
    SmoothLinearValue mPitchShifter; // 变调
    SmoothLinearValue mSpeedShifter; // 变速
};

void BaseControlSlider::prepareToPlay(double samplerate, int numChannels) {}

void BaseControlSlider::processBlock(juce::AudioBuffer<float>& buffer) {}