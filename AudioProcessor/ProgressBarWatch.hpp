#pragma once
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"
// 进度条模块
#include <atomic>
#include <cstdint>
class ProgressBarWatch : public juce::Timer {

private:
    std::atomic<int64_t> mCurProgressBarSample{0}; // 当前在多少采样点的进度条处
    AudioProcessWorker& mWorker;
    float mSampleRate;
    std::atomic_bool isStop{false};

    void setTargetSeconds(float targetSeconds);

public:
    ProgressBarWatch(AudioProcessWorker& worker);

    void prepareToPlay(float sampleRate);
    void processBlock(int numSamples);

    void timerCallback() override;

    void beginProgressBar(float targetSeconds);
    void pauseProgressBar();
    ~ProgressBarWatch();
    float getCurSeconds();
};