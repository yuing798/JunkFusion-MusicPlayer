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
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        smoothedSongChangeCrossFadeMs, // 对应UI用来处理歌曲切换的时候的交叉淡化区长度的滑块
        smoothedPlayPause,             // 设置播放暂停
        smoothedPTSChangeCrossFadeMs;  // 用来设置进度条调整的时候的交叉淡化区长度
    // 歌曲切换的时候的时域频域不相干性要远大于进度改变的不相干性，所以两者的平滑时间应该有所差别
    juce::AudioBuffer<float> fadeInSinTable;   // 淡入专用
    juce::AudioBuffer<float> fadeOutCosTable;  // 淡出专用
    std::atomic<int> currentCrossFadeIndex{0}; // 交叉淡化区的查表索引
    std::atomic<bool> isCrossFade{false};      // 是否在交叉淡化区

    struct SongChangeDuck { // 针对歌曲切换的时候的交叉淡化处理
        juce::AudioBuffer<float> tempBuffer;
        std::unique_ptr<FFmpegDecoder> decoder;
        std::unique_ptr<AudioRingBuffer> ringBuffer;

        // FadeState fadeState{FadeState::beginFadeIn};
    };
    std::array<SongChangeDuck, 2> mDucks;
    std::atomic<int> mainPlayDuckIndex{1}; // 主要是哪个duck在工作

    double mSampleRate{44100.0};

    std::atomic<bool> isFullMute{true};    // 当前是否处于完全静音状态
    std::atomic<bool> isSongChange{false}; // 不同的交叉淡化情景需要不同的交叉淡化时间

    std::string currentSongPath;

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
    void seekPreferPTS(double targetSeconds); // 跳转到目标时间点
    void setFirstPlay(std::string path, double targetSeconds);
    std::function<void(void)> onIsFullMuteTrigger; // 完全静音是否触发了
};