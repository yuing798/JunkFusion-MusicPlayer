#include "./AudioPreProcess.hpp"
#include "Macro/audioMacro.hpp"
#include "Model/PlayInfo.hpp"
#include "SystemAudioControl.hpp"

#include "Utils/constants.h"
#include "Utils/mathUtils.hpp"
#include "WindowsSystemAudioControl.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

#ifdef __linux__
    #include "SystemAudioControl/LinuxSystemAudioControl.hpp"
#elif defined(__APPLE__)
    #include "SystemAudioControl/MacosSystemAudioControl.hpp"
#endif

AudioPreProcess::AudioPreProcess(AudioProcessWorker* worker) : mWorker(worker), mPlayCount(worker) {
    for (auto& duck : mDucks) {
        duck.ringBuffer = std::make_unique<AudioRingBuffer>(1000); // 中转站分配1秒
        duck.decoder = std::make_unique<FFmpegDecoder>(duck.ringBuffer.get());
        duck.decoder->sendErrorMsg = [this](std::string msg) {
            std::string errorStr = "FFmpeg解码错误:" + msg;
            juce::var obj{new juce::DynamicObject()};
            obj.getDynamicObject()->setProperty(
                AudioMacro::errorPopupWindowMsg,
                juce::String(errorStr)
            );
            auto jsonStr{juce::JSON::toString(obj).toStdString()};
            mWorker->sender->sendMessage(jsonStr);
        };
        duck.decoder->onNatureComplete = [this] {
            playNextOrPreviousSong(true);
            stopTimer();
        };
    }
    fadeInSinTable = MathUtils::generateSinTable(0.5);
    fadeOutCosTable = MathUtils::generateCosTable(0.5);

    // 停止播放，不代表现在开始完全静音
    mWorker->receiver->onPausePlay = [this]() {
        pausePlay();
        mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Pause);
        // mPlayCount.pauseCount();
    };

    // 播放新歌
    mWorker->receiver->onPlay = [this](PlayInfo info, double targetPts) {
        if (mPlayInfo.path != info.path) {
            mPlayCount.setNewSong(info.path.toStdString(), info.duration);
            SystemAudioControl::MediaMetadata data{};
            data.title = info.title.toStdString();
            data.artists = ConvertUtils::stringArrayToVector(info.artists);
            mSystemAudioControl->updateMetadata(data);
            isSongChange = true;
        } else {
            isSongChange = false;
        }

        play(info.path, targetPts);
        mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Play);

        mPlayInfo = info;
    };

    // 设置系统音频管理类
    {
#ifdef _WIN32
        mSystemAudioControl = std::make_unique<WindowsSystemAudioControl>();
#elif defined(__linux__)
        mSystemAudioControl = std::make_unique<LinuxSystemAudioControl>();
#elif defined(__APPLE__)
        mSystemAudioControl = std::make_unique<MacosSystemAudioControl>();
#endif

        mSystemAudioControl->onLog = [](SystemAudioControl::LogRank rank, std::string str) {
            auto ptr{spdlog::get(LogAudioID).get()};
            if (rank == SystemAudioControl::LogRank::Debug) {
                ptr->debug(str);
            } else if (rank == SystemAudioControl::LogRank::Info) {
                ptr->info(str);
            } else if (rank == SystemAudioControl::LogRank::Error) {
                ptr->error(str);
            }
        };

        if (mSystemAudioControl->initialize()) {
            spdlog::get(LogAudioID)->debug("系统音频同步初始化成功");
        }

        mSystemAudioControl->onPlay = [this]() {
            play(
                mPlayInfo.path.toStdString(),
                static_cast<double>(mCurrentPtsSamples) / mSampleRate
            );
            isSongChange = false;
            mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Play);
            juce::var obj{new juce::DynamicObject()};
            obj.getDynamicObject()->setProperty(AudioMacro::playStateSync, 1);
            mWorker->sender->sendMessage(juce::JSON::toString(obj).toStdString());
        };
        mSystemAudioControl->onPause = [this]() {
            pausePlay();
            mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Pause);
            juce::var obj{new juce::DynamicObject()};
            obj.getDynamicObject()->setProperty(AudioMacro::playStateSync, 0);
            mWorker->sender->sendMessage(juce::JSON::toString(obj).toStdString());
        };

        mSystemAudioControl->onNext = [this] { playNextOrPreviousSong(true); };
        mSystemAudioControl->onPrevious = [this] { playNextOrPreviousSong(false); };

        mSystemAudioControl->onFastForward = [this](int value) {
            auto targetSeconds{
                static_cast<double>(mCurrentPtsSamples) / mSampleRate + static_cast<double>(value)
            };
            if (targetSeconds >= mPlayInfo.duration) {
                playNextOrPreviousSong(true);
            } else {
                play(mPlayInfo.path, targetSeconds);
            }
        };
        mSystemAudioControl->onRewind = [this](int value) {
            auto targetSeconds{
                static_cast<double>(mCurrentPtsSamples) / mSampleRate - static_cast<double>(value)
            };
            if (targetSeconds < 0.0) {
                playNextOrPreviousSong(false);
            } else {
                play(mPlayInfo.path, targetSeconds);
            }
        };
    }
}

void AudioPreProcess::playNextOrPreviousSong(bool nextOrPrevious) {
    juce::var obj{new juce::DynamicObject()};
    auto ptr{obj.getDynamicObject()};
    jassert(ptr);
    ptr->setProperty(AudioMacro::onPlayNextOrPreviousSong, nextOrPrevious);
    mWorker->sender->sendMessage(juce::JSON::toString(obj).toStdString());
}

void AudioPreProcess::popAudioDataAndAddZero(
    juce::AudioBuffer<float>& buffer,
    int index,
    int numSamples
) {
    auto result = mDucks[index].ringBuffer->popAudioData(buffer, numSamples);
    if (result < numSamples) {
        buffer.clear(result, numSamples - result);
    }
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

    mPlayCount.prepareToPlay(sampleRate);
}

void AudioPreProcess::processBlock(juce::AudioBuffer<float>& buffer, int numSamples) {
    buffer.clear();

    if (!isCrossFade) {
        // 交叉淡化的时候禁止播放暂停逻辑
        if (isFullMute) return;
        popAudioDataAndAddZero(buffer, mainPlayDuckIndex, numSamples);
        for (int i = 0; i < numSamples; i++) {
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
        // mDucks[mainPlayDuckIndex].ringBuffer->popAudioData(mDucks[mainPlayDuckIndex].tempBuffer);
        // mDucks[!mainPlayDuckIndex].ringBuffer->popAudioData(mDucks[!mainPlayDuckIndex].tempBuffer);
        popAudioDataAndAddZero(mDucks[mainPlayDuckIndex].tempBuffer, mainPlayDuckIndex, numSamples);
        popAudioDataAndAddZero(
            mDucks[!mainPlayDuckIndex].tempBuffer,
            !mainPlayDuckIndex,
            numSamples
        );

        for (int i = 0; i < numSamples; i++) {
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
            auto sinGainValue{MathUtils::getLinearInterpolator(
                fadeInSinTable.data(),
                fadeInSinTable.size(),
                fadeProcess
            )};
            // 给副甲板使用
            auto cosGainValue{MathUtils::getLinearInterpolator(
                fadeOutCosTable.data(),
                fadeOutCosTable.size(),
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
    for (int i = 0; i < numSamples; i++) {
        if (!isFullMute) mCurrentPtsSamples++;
    }

    mPlayCount.processBlock(numSamples, isFullMute);
}

void AudioPreProcess::timerCallback() {
    double currentSeconds = mCurrentPtsSamples / mSampleRate;
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AudioMacro::currentPTS, currentSeconds);
    auto msg = juce::JSON::toString(obj).toStdString();
    mWorker->sender->sendMessage(msg);
}
void AudioPreProcess::pausePlay() { smoothedPlayPause.setTargetValue(0.0f); }

void AudioPreProcess::play(juce::String songPath, double targetPTS) {
    mainPlayDuckIndex = !mainPlayDuckIndex;
    mCurrentPtsSamples = (int)(targetPTS * mSampleRate);

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
    mDucks[mainPlayDuckIndex].decoder->play(songPath.toStdString(), targetPTS);
    // 从打开输入上下文到帧循环的时间不过几十纳秒，开新线程完全可以
    startTimerHz(30); // 30帧的进度条刷新率
}

AudioPreProcess::~AudioPreProcess() {
    mDucks[0].decoder->stopThread(100);
    mDucks[1].decoder->stopThread(100);
    if (isTimerRunning()) stopTimer();
}