#pragma once

#include "Utils/constants.h"
#include "Utils/mathUtils.hpp"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"

// 全部平滑滑块的集中存储区域
struct SmoothSliderControl {
    SmoothLinearValue masterVolume, // 主音量
        pitchShifter;

    SmoothExpValue speedShifter; // 音调变化
    OscReceiver& oscReceiver;
    AudioProcessWorker& worker;

    float sampleRate;

    SmoothSliderControl(OscReceiver& oscReceiver, AudioProcessWorker& worker);

    void prepareToPlay(float sampleRate);

    // void processBlock(int numSamples);

    // float getMasterVolume() noexcept { return masterVolume.getCurrentValue(); }
    // float getPitchShifter() noexcept { return pitchShifter.getCurrentValue(); }
    // float getSpeedShifrer() noexcept { return speedShifter.getCurrentValue(); }
    // int getSongChangeCrossFade() noexcept {
    //     return MathUtils::msToSample(SongChangeCrossFadeMs.getCurrentValue(), sampleRate);
    // }
    // int getProgressChangeCrossFade() noexcept {
    //     return MathUtils::msToSample(progressChangeCrossFadeMs.getCurrentValue(), sampleRate);
    // }
};