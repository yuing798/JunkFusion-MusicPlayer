#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include <cstdint>
#include <string>
class PlayCount {
private:
    AudioProcessWorker* mWorker;
    int64_t mTargetUpdateSample{0}; // 目标采样点数
    double mSampleRate{44100.0};
    std::string mPath;
    int64_t mCurrentSampleCount{0};

    void updatePlayCount();

public:
    PlayCount(AudioProcessWorker* w) : mWorker(w) {}

    /**
     * @brief Set the Target Update Sample object
     *
     * @param duration 总秒数
     */
    void setNewSong(std::string path, double duration);
    void prepareToPlay(double sampleRate);

    void processBlock(int numSamplesThisBuffer, bool isFullMute);

    // 停止计数
    void pauseCount();
};