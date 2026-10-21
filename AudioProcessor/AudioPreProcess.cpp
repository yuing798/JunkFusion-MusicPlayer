// #include "./AudioPreProcess.hpp"
// #include "Macro/audioMacro.hpp"
// #include "Model/PlayInfo.hpp"

// #include "SystemAudioControl.hpp"
// #include "Utils/constants.h"
// #include "Utils/mathUtils.hpp"

// #include "juce_audio_basics/juce_audio_basics.h"
// #include "juce_core/juce_core.h"
// #include "juce_core/system/juce_PlatformDefs.h"
// #include "juce_events/juce_events.h"
// #include "processSchedule/AudioProcessWorker.hpp"
// #include <algorithm>
// #include <spdlog/spdlog.h>

// AudioPreProcess::AudioPreProcess(AudioProcessWorker* worker) : mWorker(worker) {}

// void AudioPreProcess::prepareToPlay(
//     juce::AudioChannelSet outputLayout,
//     double sampleRate,
//     int maximumExpectedSamplesPerBlock
// ) {
//     mSampleRate = sampleRate;
//     for (auto& duck : mDucks) {
//         duck.decoder->prepareToPlay(outputLayout, sampleRate);
//         duck.ringBuffer->prepareToPlay(outputLayout.size(), sampleRate);
//         duck.tempBuffer.setSize(outputLayout.size(), maximumExpectedSamplesPerBlock);
//     }
// }

// void AudioPreProcess::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) {
//     buffer.clear();

//     if (!isCrossFade) {
//         // 交叉淡化的时候禁止播放暂停逻辑
//         if (isFullMute) return;
//         popAudioDataAndAddZero(buffer, mainPlayDuckIndex, numSamples);
//         for (int i = 0; i < numSamples; i++) {
//             auto currentPlayPauseGain{smoothedPlayPause.getNextValue()};
//             if (currentPlayPauseGain == 0) {
//                 isFullMute = true;
//                 stopTimer();
//             }
//             for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
//                 auto* originPtr{buffer.getWritePointer(ch)};
//                 originPtr[i] *= currentPlayPauseGain;
//             }
//         }
//     } else {
//         mDucks[0].tempBuffer.clear();
//         mDucks[1].tempBuffer.clear();
//         //
//         mDucks[mainPlayDuckIndex].ringBuffer->popAudioData(mDucks[mainPlayDuckIndex].tempBuffer);
//         //
//         mDucks[!mainPlayDuckIndex].ringBuffer->popAudioData(mDucks[!mainPlayDuckIndex].tempBuffer);
//         popAudioDataAndAddZero(mDucks[mainPlayDuckIndex].tempBuffer, mainPlayDuckIndex,
//         numSamples); popAudioDataAndAddZero(
//             mDucks[!mainPlayDuckIndex].tempBuffer,
//             !mainPlayDuckIndex,
//             numSamples
//         );

//         for (int i = 0; i < numSamples; i++) {
//             // smoothedPlayPause.getNextValue();

//             float currentCrossFadeMs{0.0f}; // 交叉淡化区长度(毫秒数)
//             if (isSongChange.load() == true) {
//                 currentCrossFadeMs = smoothedSongChangeCrossFadeMs.getNextValue();
//                 smoothedPTSChangeCrossFadeMs.getNextValue();
//             } else {
//                 currentCrossFadeMs = smoothedPTSChangeCrossFadeMs.getNextValue();
//                 smoothedSongChangeCrossFadeMs.getNextValue();
//             }
//             // if (!isCrossFade) continue;
//             if (!isCrossFade) {
//                 // 交叉淡化已经结束，剩余样本直接输出主甲板
//                 for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
//                     buffer.getWritePointer(ch)[i] =
//                         mDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)[i];
//                 }
//                 continue;
//             }

//             for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
//                 auto* originPtr{buffer.getWritePointer(ch)};
//                 auto* mainDuckPtr{mDucks[mainPlayDuckIndex].tempBuffer.getReadPointer(ch)};
//                 auto* deputyDuckPtr{mDucks[!mainPlayDuckIndex].tempBuffer.getReadPointer(ch)};
//                 originPtr[i] = mainDuckPtr[i] * sinGainValue + deputyDuckPtr[i] * cosGainValue;
//             }
//             currentCrossFadeIndex++;
//             if (fadeProcess >= 1.0f) {
//                 mDucks[!mainPlayDuckIndex].decoder->signalThreadShouldExit();
//                 currentCrossFadeIndex = 0;
//                 isCrossFade = false;
//             }
//         }
//     }
//     for (int i = 0; i < numSamples; i++) {
//         if (!isFullMute) mCurrentPtsSamples++;
//     }

//     mPlayCount.processBlock(numSamples, isFullMute);
// }

// void AudioPreProcess::pausePlay() { smoothedPlayPause.setTargetValue(0.0f); }

// void AudioPreProcess::play(juce::String songPath, double targetPTS) {
//     mainPlayDuckIndex = !mainPlayDuckIndex;
//     mCurrentPtsSamples = (int)(targetPTS * mSampleRate);

//     if (mDucks[!mainPlayDuckIndex].decoder->isThreadRunning()) {
//         // 另一个甲板正在工作

//         if (isFullMute) {
//             // 如果另一个甲板正在工作但是是静音状态
//             isCrossFade = false;
//             if (targetPTS == 0.0) {
//                 smoothedPlayPause.setCurrentAndTargetValue(1.0f);
//             } else {
//                 smoothedPlayPause.setTargetValue(1.0f);
//                 // 歌曲不在开头的话肯定要淡入淡出的
//             }

//             // 如果另一个甲板在播放，需要平滑静音后再在processBlock中发生停止线程信号
//             mDucks[!mainPlayDuckIndex].decoder->signalThreadShouldExit();
//         } else {
//             // 如果另一个甲板正在工作且位于播放状态
//             isCrossFade = true;
//         }
//     } else {
//         isCrossFade = false; // 另一个甲板不在工作不用交叉淡化处理
//         if (isFullMute) {    // 另一个甲板不在工作且不处于完全静音的情况完全不可能发生

//             if (targetPTS == 0.0) {
//                 smoothedPlayPause.setCurrentAndTargetValue(1.0f);
//             } else {
//                 smoothedPlayPause.setTargetValue(1.0f);
//                 // 歌曲不在开头的话肯定要淡入淡出的
//             }
//         }
//     }

//     isFullMute = false;

//     // 这个值只有交叉淡化才会使用，不过设置一个int值开销小的离谱，所以放在这里是无所谓的
//     currentCrossFadeIndex = 0;
//     mDucks[mainPlayDuckIndex].ringBuffer->reset();
//     mDucks[mainPlayDuckIndex].decoder->play(songPath.toStdString(), targetPTS);
//     // 从打开输入上下文到帧循环的时间不过几十纳秒，开新线程完全可以
//     startTimerHz(30); // 30帧的进度条刷新率
// }

// AudioPreProcess::~AudioPreProcess() {
//     mDucks[0].decoder->stopThread(100);
//     mDucks[1].decoder->stopThread(100);
//     if (isTimerRunning()) stopTimer();
// }