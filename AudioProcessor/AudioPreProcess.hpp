#pragma once

#include "AudioRingBuffer.hpp"
#include "Model/PlayInfo.hpp"
#include "PlayCount.hpp"
#include "SystemAudioControl.hpp"
#include "Utils/constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

// 该类实现音频的预处理，包括解码和双甲板的交叉淡化逻辑,播放暂停，切歌等逻辑
class AudioPreProcess : public juce::Timer {
private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        smoothedSongChangeCrossFadeMs, // 对应UI用来处理歌曲切换的时候的交叉淡化区长度的滑块
        smoothedPlayPause,             // 设置播放暂停
        smoothedPTSChangeCrossFadeMs;  // 用来设置进度条调整的时候的交叉淡化区长度
    // 歌曲切换的时候的时域频域不相干性要远大于进度改变的不相干性，所以两者的平滑时间应该有所差别
    std::vector<float> fadeInSinTable;         // 淡入专用
    std::vector<float> fadeOutCosTable;        // 淡出专用
    std::atomic<int> currentCrossFadeIndex{0}; // 交叉淡化区的查表索引
    std::atomic<bool> isCrossFade{false};      // 是否在交叉淡化区

    struct Duck { // 针对交叉淡化处理
        juce::AudioBuffer<float> tempBuffer;
        std::unique_ptr<FFmpegDecoder> decoder;
        std::unique_ptr<AudioRingBuffer> ringBuffer;
    };
    std::array<Duck, 2> mDucks;            // 双甲板交叉淡化架构
    std::atomic<int> mainPlayDuckIndex{1}; // 主要是哪个duck在工作

    double mSampleRate{44100.0};

    std::atomic<bool> isFullMute{true};    // 当前是否处于完全静音状态，完全静音后才能关闭计时器
    std::atomic<bool> isSongChange{false}; // 不同的交叉淡化情景需要不同的交叉淡化时间

    AudioProcessWorker* mWorker; // 进程间调度者

    std::atomic<int64_t> mCurrentPtsSamples{0}; // 当前这首歌曲的进度条在多少个样本处(用于进度条)

    std::unique_ptr<SystemAudioControl> mSystemAudioControl;
    PlayCount mPlayCount;
    PlayInfo mPlayInfo;

public:
    AudioPreProcess(AudioProcessWorker* worker);
    ~AudioPreProcess();
    void prepareToPlay(
        juce::AudioChannelSet outputLayout,
        double sampleRate,
        int maximumExpectedSamplesPerBlock
    );
    void processBlock(juce::AudioBuffer<float>& buffer);
    void play(juce::String songPath, double targetPTS);
    void pausePlay();
    void timerCallback() override;
    bool getIsFullMute() { return isFullMute; }
};