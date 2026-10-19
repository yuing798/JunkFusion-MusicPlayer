#pragma once

#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include <atomic>
#include <cstdint>
#include <string>
class PlayCount : juce::Timer {
private:
    AudioProcessWorker& mWorker;
    int64_t mTargetUpdateSample{0}; // 目标采样点数
    double mSampleRate{44100.0};
    std::atomic<int64_t> mCurrentSampleCount{0};
    bool hadUpdate{false}; // 这首歌是否已经更新
    juce::String mPath;
    std::atomic_bool isStop{false};

    void updatePlayCount();

public:
    PlayCount(AudioProcessWorker& w) : mWorker(w) {}

    void setNewSong(juce::String path, double duration);
    void prepareToPlay(float sampleRate);

    void processBlock(int numSamples);

    void timerCallback() override;

    void continueCount();
    void pauseCount();

    DONT_COPY_AND_MOVE(PlayCount)
};