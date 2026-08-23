#pragma once

#include "AudioRingBuffer.hpp"
#include "constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include <atomic>
#include <functional>
#include <spdlog/spdlog.h>

// 该类实现音频的预处理，包括解码和双甲板的交叉淡化逻辑
class AudioPreProcess {
private:
    // std::atomic<bool> isSongChanging{false}; // 歌曲是否正在切换中，要加入平滑处理

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        smoothedSongChangeCrossFadeMs, // 对应UI用来处理交叉淡化区长度的滑块
        smoothedPlayPause;             // 设置播放暂停

    juce::AudioBuffer<float> songChangeSinTable;         // 淡入专用
    juce::AudioBuffer<float> songChangeCosTable;         // 淡出专用
    std::atomic<int> currentSongChangeCrossFadeIndex{0}; // 交叉淡化区的查表索引
    std::atomic<bool> isCrossFade{false};                // 是否在交叉淡化区

    struct SongChangeDuck { // 针对歌曲切换的时候的交叉淡化处理
        juce::AudioBuffer<float> tempBuffer;
        std::unique_ptr<FFmpegDecoder> decoder;
        std::unique_ptr<AudioRingBuffer> ringBuffer;

        // FadeState fadeState{FadeState::beginFadeIn};
    };
    std::array<SongChangeDuck, 2> mSongChangeDucks;
    std::atomic<int> mainPlayDuckIndex{1}; // 主要是哪个duck在工作

    double mSampleRate{44100.0};

    std::atomic<bool> isFullMute{true}; // 当前是否处于完全静音状态

public:
    AudioPreProcess();
    void prepareToPlay(
        juce::AudioChannelSet outputLayout,
        double sampleRate,
        int maximumExpectedSamplesPerBlock
    );
    void processBlock(juce::AudioBuffer<float>& buffer);
    void playNewSong(std::string songpath);
    void continuePlay();
    void pausePlay();
    // std::function<void(void)> onFullMuteTrigger;
    std::function<void(std::string)> sendErrorMsg;
    bool getIsFullMute() const { return isFullMute; }
};