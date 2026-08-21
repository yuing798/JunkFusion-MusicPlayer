#pragma once

#include "AudioRingBuffer.hpp"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include <atomic>

// 该类实现音频的预处理，包括解码和双甲板的交叉淡化逻辑
class AudioPreProcess {
private:
    std::atomic<bool> isSongChanging{false}; // 歌曲是否正在切换中，要加入平滑处理

    double mSmoothSongChangeMs{800.0}; // 歌曲切换或者暂停时的平滑毫秒数(0-1秒)

    struct SongChangeDuck { // 针对歌曲切换的时候的交叉淡化处理
        juce::AudioBuffer<float> tempBuffer;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
            smoothedFade; // 淡入淡出时的平滑器
        std::unique_ptr<FFmpegDecoder> decoder;
        std::unique_ptr<AudioRingBuffer> ringBuffer;
    };
    std::array<SongChangeDuck, 2> mSongChangeDucks;
    int mainPlayDuckIndex{1}; // 主要是哪个duck在工作

public:
    AudioPreProcess();
    void prepareToPlay(
        juce::AudioChannelSet outputLayout,
        double sampleRate,
        int maximumExpectedSamplesPerBlock
    );
    void processBlock(juce::AudioBuffer<float>& buffer);
    void playNewSong(std::string songpath);
};