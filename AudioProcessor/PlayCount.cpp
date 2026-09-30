#include "./PlayCount.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <string>

void PlayCount::updatePlayCount() {
    juce::var obj{new juce::DynamicObject()};
    {
        auto ptr{obj.getDynamicObject()};
        ptr->setProperty(AudioMacro::updatePlayCount, juce::String(mPath));
    }
    mWorker->sender->sendMessage(juce::JSON::toString(obj).toStdString());
    // spdlog::get(LogAudioID)->debug("通知协调者更新播放次数:路径:{}", mPath);
}

void PlayCount::setNewSong(std::string path, double duration) {
    if (mPath == path) return;
    mPath = path;
    mCurrentSampleCount = 0;
    auto targetUptateSeconds{std::min(duration * 0.5, 15.0 + 3.0 * std::sqrt(duration))};
    mTargetUpdateSample = static_cast<int>(targetUptateSeconds * mSampleRate);
}
void PlayCount::prepareToPlay(double sampleRate) { mSampleRate = sampleRate; }

void PlayCount::processBlock(int numSamplesThisBuffer, bool isFullMute) {
    if (isFullMute) return;
    mCurrentSampleCount += numSamplesThisBuffer;
    if (mCurrentSampleCount >= mTargetUpdateSample) {
        updatePlayCount();
    }
}