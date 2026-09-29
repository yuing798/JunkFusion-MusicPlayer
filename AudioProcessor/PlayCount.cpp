#include "./PlayCount.hpp"
#include "Macro/audioMacro.hpp"
#include "juce_core/juce_core.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

void PlayCount::updatePlayCount() {
    juce::var obj{new juce::DynamicObject()};
    {
        auto ptr{obj.getDynamicObject()};
        ptr->setProperty(AudioMacro::updatePlayCount, juce::String(mPath));
    }
    mWorker.sender->sendMessage(juce::JSON::toString(obj).toStdString());
}

void PlayCount::setNewSong(std::string path, double duration) {
    mPath = path;
    mCurrentSampleCount = 0;
    auto targetUptateSeconds{std::min(duration * 0.5, 15.0 + 3.0 * std::sqrt(duration))};
    mTargetUpdateSample = targetUptateSeconds * mSampleRate;
}
void PlayCount::prepareToPlay(double sampleRate) { mSampleRate = sampleRate; }

void PlayCount::processBlock(juce::AudioBuffer<float>& buffer) {
    auto numSamples{buffer.getNumSamples()};
    mCurrentSampleCount += numSamples;
    if (mCurrentSampleCount >= mTargetUpdateSample) {
        updatePlayCount();
    }
}