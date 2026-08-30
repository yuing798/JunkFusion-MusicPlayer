#include "./AudioPreProcess.hpp"
#include "AudioDefs.hpp"
#include "AudioUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"

AudioPreProcess::AudioPreProcess(AudioProcessWorker* worker) : mWorker(worker) {
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
            mWorker->sender->sendMessage(jsonStr);
        };
        duck.decoder->onNatureComplete = [this] {
            juce::var obj{new juce::DynamicObject()};
            auto ptr{obj.getDynamicObject()};
            jassert(ptr);
            ptr->setProperty(AudioDefs::onPlayNextSong, "");
            mWorker->sender->sendMessage(juce::JSON::toString(obj).toStdString());
            stopTimer();
        };
    }
    fadeInSinTable = AudioUtils::generateSinTable(0.5);
    fadeOutCosTable = AudioUtils::generateCosTable(0.5);

    // 播放新歌
    mWorker->receiver->onPlay = [this](std::string songPath, double targetPTS) {
        play(songPath, targetPTS);
    };

    // 停止播放
    mWorker->receiver->onPausePlay = [this]() { pausePlay(); };
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
            if (currentPlayPauseGain == 0) {
                isFullMute = true;
                stopTimer();
            }
            for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
                auto* originPtr{buffer.getWritePointer(ch)};
                originPtr[i] *= currentPlayPauseGain;
            }
        }
    } else {
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
            int currentCrossFadeSamples{
                static_cast<int>(currentCrossFadeMs * mSampleRate / 1000.0f)
            };

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
    for (int i = 0; i < buffer.getNumSamples(); i++) {
        if (!isFullMute) mCurrentPlaySamples++;
    }
}

void AudioPreProcess::timerCallback() {
    double currentSeconds = mCurrentPlaySamples / mSampleRate;
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AudioDefs::currentPTS, currentSeconds);
    auto msg = juce::JSON::toString(obj).toStdString();
    mWorker->sender->sendMessage(msg);
}
void AudioPreProcess::pausePlay() {
    // spdlog::get(LogAudioID)->debug("AudioPreProcess准备暂停播放");

    smoothedPlayPause.setTargetValue(0.0f);
}

void AudioPreProcess::play(std::string songPath, double targetPTS) {
    mainPlayDuckIndex = !mainPlayDuckIndex;
    mCurrentPlaySamples = (int)(targetPTS * mSampleRate);
    if (currentSongPath != songPath) {
        isSongChange = true;
    } else {
        isSongChange = false;
    } // 这里的isSongChange是为了实施不同的交叉淡化时长的，
    // 歌曲切换的不相干性远大于进度条切换，所以应该长交叉淡化
    currentSongPath = songPath;

    if (mDucks[!mainPlayDuckIndex].decoder->isThreadRunning()) {
        // 另一个甲板正在工作

        if (isFullMute) {
            // 如果另一个甲板正在工作但是是静音状态
            isCrossFade = false;
            if (targetPTS == 0.0) {
                smoothedPlayPause.setCurrentAndTargetValue(1.0f);
            } else {
                smoothedPlayPause.setTargetValue(1.0f);
                // 歌曲不在开头的话肯定要淡入淡出的
            }

            // 如果另一个甲板在播放，需要平滑静音后再在processBlock中发生停止线程信号
            mDucks[!mainPlayDuckIndex].decoder->signalThreadShouldExit();
        } else {
            // 如果另一个甲板正在工作且位于播放状态
            isCrossFade = true;
        }
    } else {
        isCrossFade = false; // 另一个甲板不在工作不用交叉淡化处理
        if (isFullMute) {    // 另一个甲板不在工作且不处于完全静音的情况完全不可能发生

            if (targetPTS == 0.0) {
                smoothedPlayPause.setCurrentAndTargetValue(1.0f);
            } else {
                smoothedPlayPause.setTargetValue(1.0f);
                // 歌曲不在开头的话肯定要淡入淡出的
            }
        }
    }

    isFullMute = false;

    // 这个值只有交叉淡化才会使用，不过设置一个int值开销小的离谱，所以放在这里是无所谓的
    currentCrossFadeIndex = 0;
    mDucks[mainPlayDuckIndex].ringBuffer->reset();
    mDucks[mainPlayDuckIndex].decoder->play(songPath, targetPTS);
    // 从打开输入上下文到帧循环的时间不过几十纳秒，开新线程完全可以
    startTimerHz(30); // 30帧的进度条刷新率
}

AudioPreProcess::~AudioPreProcess() {
    mDucks[0].decoder->stopThread(100);
    mDucks[1].decoder->stopThread(100);
    if (isTimerRunning()) stopTimer();
}