#include "./Stretch.hpp"
#include "Utils/constants.h"
#include "Utils/otherUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <spdlog/spdlog.h>

Stretch::Stretch(AudioProcessWorker* w, OscReceiver* o) : mWorker(w), mOscReceiver(o) {
    auto onPitchShifterChange = [this](float value) {
        spdlog::get(LogAudioID)->debug("[Stretch::Stretch]准备调节变调量至目标值:{}", value);
        mPitchShifter.setTargetValue(value);
    };
    auto onSpeedShifterChange = [this](float value) {
        spdlog::get(LogAudioID)->debug("[Stretch::Stretch]准备调节变速量至目标值:{}", value);
        mSpeedShifter.setTargetValue(value);
    };
    mWorker->receiver->onPitchShifterChange = onPitchShifterChange;
    mOscReceiver->onPitchShifterChange = onPitchShifterChange;
    mWorker->receiver->onSpeedShifterChange = onSpeedShifterChange;
    mOscReceiver->onSpeedShifterChange = onSpeedShifterChange;
}

void Stretch::prepareToPlay(double samplerate, int numChannels, int maxNumSamples) {
    mPitchShifter.reset(samplerate, 0.002);
    mSpeedShifter.reset(samplerate, 0.002);
    mPitchShifter.setCurrentAndTargetValue(0.0f);
    mSpeedShifter.setCurrentAndTargetValue(1.0f);
    mStretch.presetDefault(numChannels, (float)samplerate);
    mStretch.setTransposeSemitones(mPitchShifter.getCurrentValue());
    tempBuffer.setSize(numChannels, maxNumSamples * 2);
    tempBuffer.clear();
    mRingBuffer.prepareToPlay(numChannels, samplerate);
}

void Stretch::processBlock(
    juce::AudioBuffer<float>& inputBuffer,
    int beforeFifoNumSamples,
    juce::AudioBuffer<float>& outputBuffer,
    int finalNumSamples
) {
    mSpeedShifter.skip(finalNumSamples);
    mPitchShifter.skip(finalNumSamples);
    auto pitchShiftValue{mPitchShifter.getCurrentValue()};
    auto speedShiftValue{mSpeedShifter.getCurrentValue()};
    mStretch.setTransposeSemitones(pitchShiftValue);

    auto numForPush{juce::jmin(beforeFifoNumSamples, mRingBuffer.getFreeSpace())};
    if (numForPush > 0) {
        mRingBuffer.pushAudioData(inputBuffer, numForPush);
    }
    // 推入变速变调算法需要的数据量
    auto inputNeed{static_cast<int>(finalNumSamples * speedShiftValue)};
    inputNeed = juce::jlimit(0, tempBuffer.getNumSamples(), inputNeed);
    if (mRingBuffer.getNumReady() < inputNeed) {
        // OtherUtils::writeEmergencyLog(
        //     std::string("环形缓冲区数据不足，提前返回") + "\n环形缓冲区可用样本点数目" +
        //     std::to_string(mRingBuffer.getNumReady()) + "inputNeed=" + std::to_string(inputNeed)
        // );
        outputBuffer.clear();
        return;
    }
    mRingBuffer.popAudioData(tempBuffer, inputNeed);

    // if (speedShiftValue == 1.0f && pitchShiftValue == 0.0f) return;
    // for (int ch = 0; ch < outputBuffer.getNumChannels(); ch++) {
    //     outputBuffer.copyFrom(ch, 0, inputBuffer.getReadPointer(ch), inputNeed);
    // }

    mStretch.process(
        tempBuffer.getArrayOfReadPointers(),
        inputNeed,
        outputBuffer.getArrayOfWritePointers(),
        finalNumSamples
    );
}
float Stretch::getSpeedShifterValue() const noexcept { return mSpeedShifter.getCurrentValue(); }