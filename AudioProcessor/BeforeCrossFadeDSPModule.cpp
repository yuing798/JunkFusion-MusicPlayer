#include "./BeforeCrossFadeDSPModule.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"

BeforeCrossFadeDSPModule::BeforeCrossFadeDSPModule(
    AudioProcessWorker& worker,
    SmoothSliderControl& smoothSlider
)
    : mWorker(worker), mSmoothSlider(smoothSlider), mDecoder(&mRingBuffer1),
      mStretch(mSmoothSlider) {
    mDecoder.sendErrorMsg = [this](std::string msg) {
        std::string errorStr = "FFmpeg解码错误:" + msg;
        Yvar obj = Yvar::object();
        obj.set(AudioMacro::errorPopupWindowMsg, errorStr);
        auto jsonStr{obj.dump()};
        mWorker.sender->sendMessage(jsonStr);
    };
    mDecoder.onNatureComplete = [this]() {
        if (onRequestPlayNextSong) onRequestPlayNextSong();
    };
}

void BeforeCrossFadeDSPModule::prepareToPlay(
    float sampleRate,
    juce::AudioChannelSet channelSet,
    int maxBufferCapacity
) {
    // 因为变速模块的最大倍速为2,所以分配两倍大小
    mDecoder.prepareToPlay(channelSet, sampleRate);
    mRingBuffer1.prepareToPlay(channelSet.size(), sampleRate);
    mSpeedShifterBuffer.setSize(channelSet.size(), maxBufferCapacity * kMaxSpeedShiftValue);
    mStretch.prepareToPlay(sampleRate, channelSet.size(), maxBufferCapacity);
}

void BeforeCrossFadeDSPModule::popAudioDataAndAddZero(
    juce::AudioBuffer<float>& buffer,
    int numSamples
) {

    auto result = mRingBuffer1.popAudioData(buffer, numSamples);
    if (result < numSamples) {
        buffer.clear(result, numSamples - result);
    }
}

void BeforeCrossFadeDSPModule::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) {
    // 不需要持有isFullMute位，因为静音的话直接改变传入的numSamples就行
    mSpeedShifterBuffer.clear(); // 这里是DSP链路开始的第一环
    buffer.clear();
    if (isFullMute) {
        return;
    }
    int numSamplesBeforeStretch = numSamples * mSmoothSlider.speedShifter.getCurrentValue();
    popAudioDataAndAddZero(mSpeedShifterBuffer, numSamplesBeforeStretch);
    mStretch.processBlock(mSpeedShifterBuffer, numSamplesBeforeStretch, buffer, numSamples);
}
void BeforeCrossFadeDSPModule::beginNewDecoderThread(std::string songPath, double targetSeconds) {
    reset();
    mDecoder.play(songPath, targetSeconds);
    isFullMute = false;
}
void BeforeCrossFadeDSPModule::continueThread() { isFullMute = false; }
void BeforeCrossFadeDSPModule::pauseThread() { isFullMute = true; }
void BeforeCrossFadeDSPModule::reset() {
    mDecoder.stopThread(200);
    mStretch.reset();
    mRingBuffer1.reset();
}