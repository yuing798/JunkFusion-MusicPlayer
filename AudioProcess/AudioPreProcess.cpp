#include "./AudioPreProcess.hpp"
#include "AudioUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"

AudioPreProcess::AudioPreProcess() {
    for (auto& duck : mSongChangeDucks) {
        duck.ringBuffer = std::make_unique<AudioRingBuffer>(1000); // 中转站分配1秒
        duck.decoder = std::make_unique<FFmpegDecoder>(duck.ringBuffer.get());
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
        // duck.smoothedFade.reset(sampleRate, mSmoothSongChangeMs / 1000.0);
        // duck.smoothedFade.setCurrentAndTargetValue(0.0f);
        duck.tempBuffer.setSize(outputLayout.size(), maximumExpectedSamplesPerBlock);
    }
    smoothedSongChangeCrossFadeMs.reset(sampleRate, 0.002f);
    smoothedSongChangeCrossFadeMs.setCurrentAndTargetValue(500.0f); // 默认500毫秒长度的交叉淡化区
}

void AudioPreProcess::processBlock(juce::AudioBuffer<float>& buffer) {
    buffer.clear();
    mSongChangeDucks[0].tempBuffer.clear();
    mSongChangeDucks[1].tempBuffer.clear();

    if (isCrossFade) {
        mSongChangeDucks[mainPlayDuckIndex].ringBuffer->popAudioData(
            mSongChangeDucks[mainPlayDuckIndex].tempBuffer
        );
        mSongChangeDucks[!mainPlayDuckIndex].ringBuffer->popAudioData(
            mSongChangeDucks[!mainPlayDuckIndex].tempBuffer
        );
    } else {
        mSongChangeDucks[mainPlayDuckIndex].ringBuffer->popAudioData(buffer);
    }

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        float currentSongChangeCrossFadeMs =
            smoothedSongChangeCrossFadeMs.getNextValue(); // 交叉淡化区长度(毫秒数)
        if (!isCrossFade) continue;
        // 交叉淡化区域的实际样本数
        int currentSongChangeCrossFadeSamples =
            currentSongChangeCrossFadeMs * mSampleRate / 1000.0f;
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
        if (isCrossFade) {

            for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                auto* originPtr{buffer.getWritePointer(ch)};
                auto* mainDuckPtr{
                    mSongChangeDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)
                };
                auto* deputyDuckPtr{
                    mSongChangeDucks[!mainPlayDuckIndex].tempBuffer.getReadPointer(ch)
                };
                originPtr[i] = mainDuckPtr[i] * sinGainValue + deputyDuckPtr[i] * cosGainValue;
            }
            currentSongChangeCrossFadeIndex++;
            if (fadeProcess >= 1.0f) {
                mSongChangeDucks[!mainPlayDuckIndex].decoder->stopThread(200);
                currentSongChangeCrossFadeIndex = 0;
                isCrossFade = false;
            }
        }
    }
}

void AudioPreProcess::playNewSong(std::string songPath) {
    mainPlayDuckIndex = !mainPlayDuckIndex;

    mSongChangeDucks[mainPlayDuckIndex].ringBuffer->reset();
    mSongChangeDucks[mainPlayDuckIndex].decoder->playNewSong(songPath);
    isCrossFade = true;
    currentSongChangeCrossFadeIndex = 0;
}