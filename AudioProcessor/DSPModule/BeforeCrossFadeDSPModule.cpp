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
    // ── 先把别的线程请求的重置落下去 ──
    //
    // 必须在最前面（在 isFullMute 提前返回之前）：静止的 deck 也要能被清干净。
    // 放在音频线程做是**必须**的：fifo 的读写位置和 stretch 的内部状态都不能
    // 被另一个线程并发改，否则 prepareToRead 会给出越界下标（见 reset() 注释）。
    if (mResetPending.exchange(false, std::memory_order_acq_rel)) {
        mStretch.reset();
        mRingBuffer1.reset();
    }

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
    // 先让生产者停下来（解码线程不是音频线程，这里等它是安全的）
    mDecoder.stopThread(200);

    // 再请求音频线程清空消费端，并等它落地：
    // 解码线程随后就要往里写数据，不先清干净的话开头那几毫秒会被清掉
    mResetPending.store(true, std::memory_order_release);
    for (int i = 0; i < 100 && mResetPending.load(std::memory_order_acquire); ++i) {
        // 设备没在跑（没有回调）时不无限等，直接往下走
        juce::Thread::sleep(1);
    }
}

void BeforeCrossFadeDSPModule::resetOnAudioThread() {
    // 已经在音频线程上了，直接改是安全的；顺手把待处理标记清掉
    mStretch.reset();
    mRingBuffer1.reset();
    mResetPending.store(false, std::memory_order_release);
}