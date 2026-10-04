#include "./Stretch.hpp"
#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"

Stretch::Stretch(AudioProcessWorker* w, OscReceiver* o) : mWorker(w), mOscReceiver(o) {}

void Stretch::prepareToPlay(double samplerate, int numChannels) {
    mPitchShifter.reset(samplerate, 0.002);
    mSpeedShifter.reset(samplerate, 0.002);
    mPitchShifter.setCurrentAndTargetValue(0.0f);
    mSpeedShifter.setCurrentAndTargetValue(1.0f);
    mStretch.presetDefault(numChannels, samplerate);
    mStretch.setTransposeSemitones(mPitchShifter.getCurrentValue());
}

void Stretch::processBlock(
    juce::AudioBuffer<float>& inputBuffer,
    juce::AudioBuffer<float>& outputBuffer
) {
    auto outputSamples{outputBuffer.getNumSamples()};
    mSpeedShifter.skip(outputSamples);
    mPitchShifter.skip(outputSamples);
    auto pitchShiftValue{mPitchShifter.getCurrentValue()};
    auto speedShiftValue{mSpeedShifter.getCurrentValue()};
    mStretch.setTransposeSemitones(pitchShiftValue);

    auto numForPush{juce::jmin(inputBuffer.getNumSamples(), mRingBuffer.getFreeSpace())};
    if (numForPush > 0) {
        mRingBuffer.pushAudioData(inputBuffer, numForPush);
    }
    // 推入变速变调算法需要的数据量
    auto inputNeed{static_cast<int>(outputSamples * speedShiftValue) + 1};
    inputNeed = juce::jlimit(0, tempBuffer.getNumSamples(), inputNeed);
    if (mRingBuffer.getNumReady() < inputNeed) {
        outputBuffer.clear();
        return;
    }
    mRingBuffer.popAudioData(tempBuffer, inputNeed);

    if (speedShiftValue == 1.0f && pitchShiftValue == 0.0f) return;

    mStretch.process(tempBuffer, outputSamples * speedShiftValue, outputBuffer, outputSamples);
}
float Stretch::getSpeedShifterValue() const noexcept { return mSpeedShifter.getCurrentValue(); }