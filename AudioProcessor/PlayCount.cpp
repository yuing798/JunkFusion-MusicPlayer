#include "./PlayCount.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <string>

void PlayCount::updatePlayCount() {
    Yvar obj = Yvar::object();
    obj.set(AudioMacro::updatePlayCount, mPath);
    mWorker.sender->sendMessage(obj.dump());
    // spdlog::get(LogAudioID)->debug("通知协调者更新播放次数:路径:{}", mPath);
}

void PlayCount::setNewSong(std::string path, double duration) {
    if (mPath == path) return;
    mPath = path;
    mCurrentSampleCount = 0;
    hadUpdate = false;
    auto targetUptateSeconds{std::min(duration * 0.5, 15.0 + 3.0 * std::sqrt(duration))};
    mTargetUpdateSample = static_cast<int>(targetUptateSeconds * mSampleRate);
    continueCount();
}
void PlayCount::prepareToPlay(float sampleRate) { mSampleRate = sampleRate; }

void PlayCount::processBlock(int numSamples) { mCurrentSampleCount += isStop ? 0 : numSamples; }

void PlayCount::timerCallback() {
    if (mCurrentSampleCount >= mTargetUpdateSample && !hadUpdate) {
        updatePlayCount();
        hadUpdate = true;
    }
}
void PlayCount::continueCount() {
    if (!isTimerRunning()) startTimerHz(2);
    isStop = false;
}
void PlayCount::pauseCount() {
    if (isTimerRunning()) stopTimer();
    isStop = true;
}