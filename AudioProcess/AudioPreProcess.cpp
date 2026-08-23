#include "./AudioPreProcess.hpp"
#include "AudioDefs.hpp"
#include "AudioUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_events/juce_events.h"

AudioPreProcess::AudioPreProcess() {
    for (auto& duck : mSongChangeDucks) {
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
    songChangeSinTable = AudioUtils::generateSinTable(0.5);
    songChangeCosTable = AudioUtils::generateCosTable(0.5);
}
void AudioPreProcess::prepareToPlay(
    juce::AudioChannelSet outputLayout,
    double sampleRate,
    int maximumExpectedSamplesPerBlock
) {
    mSampleRate = sampleRate;
    for (auto& duck : mSongChangeDucks) {
        duck.decoder->prepareToPlay(outputLayout, sampleRate);
        duck.ringBuffer->prepareToPlay(outputLayout.size(), sampleRate);
        duck.tempBuffer.setSize(outputLayout.size(), maximumExpectedSamplesPerBlock);
    }
    smoothedSongChangeCrossFadeMs.reset(sampleRate, 0.002f);
    smoothedSongChangeCrossFadeMs.setCurrentAndTargetValue(300.0f); // 默认长度的交叉淡化区
    smoothedPlayPause.reset(sampleRate, 0.5f);
    smoothedPlayPause.setCurrentAndTargetValue(0.0f);
}

void AudioPreProcess::processBlock(juce::AudioBuffer<float>& buffer) {
    buffer.clear();

    if (!isCrossFade) {
        // 交叉淡化的时候禁止播放暂停逻辑
        if (isFullMute) return;
        mSongChangeDucks[mainPlayDuckIndex].ringBuffer->popAudioData(buffer);
        for (int i = 0; i < buffer.getNumSamples(); i++) {
            smoothedSongChangeCrossFadeMs.getNextValue();
            auto currentPlayPauseGain{smoothedPlayPause.getNextValue()};
            if (currentPlayPauseGain == 0) isFullMute = true;
            for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                auto* originPtr{buffer.getWritePointer(ch)};
                originPtr[i] *= currentPlayPauseGain;
            }
        }
        return;
    }
    mSongChangeDucks[0].tempBuffer.clear();
    mSongChangeDucks[1].tempBuffer.clear();
    mSongChangeDucks[mainPlayDuckIndex].ringBuffer->popAudioData(
        mSongChangeDucks[mainPlayDuckIndex].tempBuffer
    );
    mSongChangeDucks[!mainPlayDuckIndex].ringBuffer->popAudioData(
        mSongChangeDucks[!mainPlayDuckIndex].tempBuffer
    );

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        // smoothedPlayPause.getNextValue();
        float currentSongChangeCrossFadeMs =
            smoothedSongChangeCrossFadeMs.getNextValue(); // 交叉淡化区长度(毫秒数)
        // if (!isCrossFade) continue;
        if (!isCrossFade) {
            // 交叉淡化已经结束，剩余样本直接输出主甲板
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                buffer.getWritePointer(ch)[i] =
                    mSongChangeDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)[i];
            }
            continue;
        }
        // 交叉淡化区域的实际样本数
        int currentSongChangeCrossFadeSamples{
            static_cast<int>(currentSongChangeCrossFadeMs * mSampleRate / 1000.0f)
        };

        // 交叉淡化程度
        float fadeProcess{
            (float)currentSongChangeCrossFadeIndex / (float)currentSongChangeCrossFadeSamples
        };
        // 给主甲板使用
        auto sinGainValue{AudioUtils::getLinearInterpolator(
            songChangeSinTable.getReadPointer(0),
            songChangeSinTable.getNumSamples(),
            fadeProcess
        )};
        // 给副甲板使用
        auto cosGainValue{AudioUtils::getLinearInterpolator(
            songChangeCosTable.getReadPointer(0),
            songChangeCosTable.getNumSamples(),
            fadeProcess
        )};
        // if (isCrossFade) {

        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto* originPtr{buffer.getWritePointer(ch)};
            auto* mainDuckPtr{mSongChangeDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)};
            auto* deputyDuckPtr{mSongChangeDucks[!mainPlayDuckIndex].tempBuffer.getReadPointer(ch)};
            originPtr[i] = mainDuckPtr[i] * sinGainValue + deputyDuckPtr[i] * cosGainValue;
        }
        currentSongChangeCrossFadeIndex++;
        if (fadeProcess >= 1.0f) {
            mSongChangeDucks[!mainPlayDuckIndex].decoder->signalThreadShouldExit();
            currentSongChangeCrossFadeIndex = 0;
            isCrossFade = false;
        }
        // }
    }
}

void AudioPreProcess::playNewSong(std::string songPath) {

    mainPlayDuckIndex = !mainPlayDuckIndex;

    if (!mSongChangeDucks[!mainPlayDuckIndex].decoder->isThreadRunning()) {
        // 只有主甲板在工作，副甲板完全没有在播放歌曲
        smoothedPlayPause.setCurrentAndTargetValue(1.0f);
        isFullMute = false;
        isCrossFade = false;
    } else if (isFullMute == true) {
        // 副甲板在工作但是当前处于暂停状态
        mSongChangeDucks[!mainPlayDuckIndex]
            .decoder->signalThreadShouldExit(); // 直接把副甲板的线程停止
        isCrossFade = false;
        isFullMute = false;
        smoothedPlayPause.setCurrentAndTargetValue(1.0f); // 歌曲刚开始的时候应该不需要平滑进入

    } else {
        // 副甲板在工作且播放的时候突然切歌
        isCrossFade = true;
    }
    mSongChangeDucks[mainPlayDuckIndex].ringBuffer->reset();
    mSongChangeDucks[mainPlayDuckIndex].decoder->playNewSong(songPath);
    currentSongChangeCrossFadeIndex = 0;
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