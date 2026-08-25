#include "./AudioPreProcess.hpp"
#include "AudioDefs.hpp"
#include "AudioUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_events/juce_events.h"

AudioPreProcess::AudioPreProcess() {
    for (auto& duck : mDucks) {
        duck.ringBuffer = std::make_unique<AudioRingBuffer>(1000); // 中转站分配1秒
        duck.decoder = std::make_unique<FFmpegDecoder>(duck.ringBuffer.get());
        duck.decoder->sendErrorMsg = [this](std::string msg) {
            std::string errorStr = "FFmpeg解码错误:" + msg;
            juce::var obj{new juce::DynamicObject()};
            obj.getDynamicObject()->setProperty(
                AudioDefs::errorPopupWindowMsg,
                juce::String(errorStr)
            );
            auto jsonStr{juce::JSON::toString(obj).toStdString()};
            if (sendErrorMsg) sendErrorMsg(jsonStr);
        };
    }
    fadeInSinTable = AudioUtils::generateSinTable(0.5);
    fadeOutCosTable = AudioUtils::generateCosTable(0.5);
}
void AudioPreProcess::prepareToPlay(
    juce::AudioChannelSet outputLayout,
    double sampleRate,
    int maximumExpectedSamplesPerBlock
) {
    mSampleRate = sampleRate;
    for (auto& duck : mDucks) {
        duck.decoder->prepareToPlay(outputLayout, sampleRate);
        duck.ringBuffer->prepareToPlay(outputLayout.size(), sampleRate);
        duck.tempBuffer.setSize(outputLayout.size(), maximumExpectedSamplesPerBlock);
    }
    smoothedSongChangeCrossFadeMs.reset(sampleRate, 0.002f);
    smoothedSongChangeCrossFadeMs.setCurrentAndTargetValue(800.0f); // 默认长度的交叉淡化区
    smoothedPTSChangeCrossFadeMs.reset(sampleRate, 0.002f);
    smoothedPTSChangeCrossFadeMs.setCurrentAndTargetValue(40.0f);
    smoothedPlayPause.reset(sampleRate, 0.5f);
    smoothedPlayPause.setCurrentAndTargetValue(0.0f);
}

void AudioPreProcess::processBlock(juce::AudioBuffer<float>& buffer) {
    buffer.clear();

    if (!isCrossFade) {
        // 交叉淡化的时候禁止播放暂停逻辑
        if (isFullMute) return;
        mDucks[mainPlayDuckIndex].ringBuffer->popAudioData(buffer);
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            smoothedSongChangeCrossFadeMs.getNextValue();
            smoothedPTSChangeCrossFadeMs.getNextValue();
            auto currentPlayPauseGain{smoothedPlayPause.getNextValue()};
            if (currentPlayPauseGain == 0) isFullMute = true;
            for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                auto* originPtr{buffer.getWritePointer(ch)};
                originPtr[i] *= currentPlayPauseGain;
            }
        }
        return;
    }
    mDucks[0].tempBuffer.clear();
    mDucks[1].tempBuffer.clear();
    mDucks[mainPlayDuckIndex].ringBuffer->popAudioData(mDucks[mainPlayDuckIndex].tempBuffer);
    mDucks[!mainPlayDuckIndex].ringBuffer->popAudioData(mDucks[!mainPlayDuckIndex].tempBuffer);

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        // smoothedPlayPause.getNextValue();

        float currentCrossFadeMs{0.0f}; // 交叉淡化区长度(毫秒数)
        if (isSongChange.load() == true) {
            currentCrossFadeMs = smoothedSongChangeCrossFadeMs.getNextValue();
            smoothedPTSChangeCrossFadeMs.getNextValue();
        } else {
            currentCrossFadeMs = smoothedPTSChangeCrossFadeMs.getNextValue();
            smoothedSongChangeCrossFadeMs.getNextValue();
        }
        // if (!isCrossFade) continue;
        if (!isCrossFade) {
            // 交叉淡化已经结束，剩余样本直接输出主甲板
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                buffer.getWritePointer(ch)[i] =
                    mDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)[i];
            }
            continue;
        }
        // 交叉淡化区域的实际样本数
        int currentCrossFadeSamples{static_cast<int>(currentCrossFadeMs * mSampleRate / 1000.0f)};

        // 交叉淡化程度
        float fadeProcess{(float)currentCrossFadeIndex / (float)currentCrossFadeSamples};
        // 给主甲板使用
        auto sinGainValue{AudioUtils::getLinearInterpolator(
            fadeInSinTable.getReadPointer(0),
            fadeInSinTable.getNumSamples(),
            fadeProcess
        )};
        // 给副甲板使用
        auto cosGainValue{AudioUtils::getLinearInterpolator(
            fadeOutCosTable.getReadPointer(0),
            fadeOutCosTable.getNumSamples(),
            fadeProcess
        )};

        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto* originPtr{buffer.getWritePointer(ch)};
            auto* mainDuckPtr{mDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)};
            auto* deputyDuckPtr{mDucks[!mainPlayDuckIndex].tempBuffer.getReadPointer(ch)};
            originPtr[i] = mainDuckPtr[i] * sinGainValue + deputyDuckPtr[i] * cosGainValue;
        }
        currentCrossFadeIndex++;
        if (fadeProcess >= 1.0f) {
            mDucks[!mainPlayDuckIndex].decoder->signalThreadShouldExit();
            currentCrossFadeIndex = 0;
            isCrossFade = false;
        }
    }
}

void AudioPreProcess::playNewSong(std::string songPath) {

    mainPlayDuckIndex = !mainPlayDuckIndex;

    if (!mDucks[!mainPlayDuckIndex].decoder->isThreadRunning()) {
        // 只有主甲板在工作，副甲板完全没有在播放歌曲
        smoothedPlayPause.setCurrentAndTargetValue(1.0f);
        isFullMute = false;
        isCrossFade = false;
    } else if (isFullMute == true) {
        // 副甲板在工作但是当前处于暂停状态
        mDucks[!mainPlayDuckIndex].decoder->signalThreadShouldExit(); // 直接把副甲板的线程停止
        isCrossFade = false;
        isFullMute = false;
        smoothedPlayPause.setCurrentAndTargetValue(1.0f); // 歌曲刚开始的时候应该不需要平滑进入

    } else {
        // 副甲板在工作且播放的时候突然切歌
        isCrossFade = true;
    }
    isSongChange = true;
    mDucks[mainPlayDuckIndex].ringBuffer->reset();
    currentSongPath = songPath;
    mDucks[mainPlayDuckIndex].decoder->playNewSong(songPath);
    currentCrossFadeIndex = 0;
}
void AudioPreProcess::pausePlay() {
    // spdlog::get(LogAudioID)->debug("AudioPreProcess准备暂停播放");

    smoothedPlayPause.setTargetValue(0.0f);
}
void AudioPreProcess::continuePlay() {
    // spdlog::get(LogAudioID)->debug("AudioPreProcess准备继续播放");
    smoothedPlayPause.setTargetValue(1.0f);
    isFullMute = false;
}

void AudioPreProcess::seekPreferPTS(double targetSeconds) {
    mainPlayDuckIndex = !mainPlayDuckIndex;
    if (isFullMute == false) {
        isCrossFade = true;
        mDucks[mainPlayDuckIndex].ringBuffer->reset();
        mDucks[mainPlayDuckIndex].decoder->seekPreferPTS(currentSongPath, targetSeconds);
        currentCrossFadeIndex = 0;
    }
    isSongChange = false;
}

void AudioPreProcess::setFirstPlay(std::string path, double targetSeconds) {
    isFullMute = false;
    mDucks[mainPlayDuckIndex].ringBuffer->reset();
    mDucks[mainPlayDuckIndex].decoder->seekPreferPTS(path, targetSeconds);
    isSongChange = false;
    smoothedPlayPause.setCurrentAndTargetValue(1.0f);
    currentSongPath = path;
}