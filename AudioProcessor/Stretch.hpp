#pragma once

#include "AudioRingBuffer.hpp"
#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include "signalsmith-stretch/signalsmith-stretch.h"
#include <memory>

// 倍速，音高偏移
class Stretch {
private:
    SmoothLinearValue mPitchShifter; // 变调
    SmoothExpValue mSpeedShifter;    // 变速,变速使用指数平滑效果更好
    signalsmith::stretch::SignalsmithStretch<float> mStretch;
    AudioProcessWorker* mWorker;
    OscReceiver* mOscReceiver;

    AudioRingBuffer mRingBuffer{500};
    juce::AudioBuffer<float> tempBuffer; // 中介缓冲区,存储FIFO输出到变速输入这一段的数据

public:
    Stretch(AudioProcessWorker* w, OscReceiver* o);
    void prepareToPlay(double samplerate, int numChannels);

    void
    processBlock(juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<float>& outputBuffer);
    /**
     * @brief Get the Speed Shifter Value
     * 因为两个fifo合作的话中间所有的音频函数模块处理的样本数都要变成原样本数*倍速率
     *
     * @return float
     */
    float getSpeedShifterValue() const noexcept;
};