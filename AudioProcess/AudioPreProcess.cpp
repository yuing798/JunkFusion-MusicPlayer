#include "./AudioPreProcess.hpp"
#include "juce_audio_basics/juce_audio_basics.h"

AudioPreProcess::AudioPreProcess() {
    for (auto& duck : mSongChangeDucks) {
        duck.ringBuffer = std::make_unique<AudioRingBuffer>(1000); // 中转站分配1秒
        duck.decoder = std::make_unique<FFmpegDecoder>(duck.ringBuffer.get());
    }
}
void AudioPreProcess::prepareToPlay(
    juce::AudioChannelSet outputLayout,
    double sampleRate,
    int maximumExpectedSamplesPerBlock
) {
    for (auto& duck : mSongChangeDucks) {
        duck.decoder->prepareToPlay(outputLayout, sampleRate);
        duck.ringBuffer->prepareToPlay(outputLayout.size(), sampleRate);
        duck.smoothedFade.reset(sampleRate, mSmoothSongChangeMs / 1000.0);
        duck.smoothedFade.setCurrentAndTargetValue(0.0f);
        duck.tempBuffer.setSize(outputLayout.size(), maximumExpectedSamplesPerBlock);
    }
}

void AudioPreProcess::processBlock(juce::AudioBuffer<float>& buffer) {
    mSongChangeDucks[0].tempBuffer.clear();
    mSongChangeDucks[1].tempBuffer.clear();

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        for (auto& duck : mSongChangeDucks) {
            float currentFadeValue = duck.smoothedFade.getNextValue();
            if (currentFadeValue == 0) {
                if (duck.decoder->isThreadRunning()) {
                    // 平滑增益变化后等于0且正在运行说明在淡出区的边缘
                    duck.decoder->stopThread(200);
                } else {
                    continue;
                }
            }
            duck.ringBuffer->popAudioData(duck.tempBuffer);
            for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                auto* tempPtr{duck.tempBuffer.getWritePointer(ch)};
                auto* originPtr{buffer.getWritePointer(ch)};
                tempPtr[i] *= currentFadeValue;
                originPtr[i] += tempPtr[i];
            }

            // if (duck.decoder->isThreadRunning()) {

            //     if (currentFadeValue == 0) {
            //         duck.decoder->stopThread(200);
            //         continue;
            //     }
            //     duck.ringBuffer->popAudioData(duck.tempBuffer);
            //     for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            //         auto* tempPtr{duck.tempBuffer.getWritePointer(ch)};
            //         auto* originPtr{buffer.getWritePointer(ch)};
            //         tempPtr[i] *= currentFadeValue;
            //         originPtr[i] += tempPtr[i];
            //     }
            // }
        }
    }
}

void AudioPreProcess::playNewSong(std::string songPath) {
    mainPlayDuckIndex = !mainPlayDuckIndex;

    mSongChangeDucks[!mainPlayDuckIndex].smoothedFade.setTargetValue(0.0f);
    mSongChangeDucks[mainPlayDuckIndex].smoothedFade.setTargetValue(1.0f);
    mSongChangeDucks[mainPlayDuckIndex].ringBuffer->reset();
    mSongChangeDucks[mainPlayDuckIndex].decoder->playNewSong(songPath);
}