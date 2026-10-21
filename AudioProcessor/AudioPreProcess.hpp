// #pragma once

// #include "AudioRingBuffer.hpp"
// #include "Utils/constants.h"
// #include "ffmpegDecoder.hpp"
// #include "juce_audio_basics/juce_audio_basics.h"
// #include "juce_core/juce_core.h"
// #include "juce_events/juce_events.h"
// #include "processSchedule/AudioProcessWorker.hpp"
// #include <atomic>
// #include <cstdint>
// #include <functional>
// #include <spdlog/spdlog.h>

// // 该类实现音频的预处理，包括解码和双甲板的交叉淡化逻辑,播放暂停，切歌等逻辑
// class AudioPreProcess {
// private:
//     std::atomic<bool> isCrossFade{false}; // 是否在交叉淡化区

//     struct Duck { // 针对交叉淡化处理
//         juce::AudioBuffer<float> tempBuffer;
//         std::unique_ptr<FFmpegDecoder> decoder;
//         std::unique_ptr<AudioRingBuffer> ringBuffer;
//     };
//     std::array<Duck, 2> mDucks;            // 双甲板交叉淡化架构
//     std::atomic<int> mainPlayDuckIndex{1}; // 主要是哪个duck在工作

//     double mSampleRate{44100.0};

//     std::atomic<bool> isFullMute{true};    // 当前是否处于完全静音状态，完全静音后才能关闭计时器
//     std::atomic<bool> isSongChange{false}; // 不同的交叉淡化情景需要不同的交叉淡化时间

//     AudioProcessWorker* mWorker; // 进程间调度者

// public:
//     AudioPreProcess(AudioProcessWorker* worker);
//     ~AudioPreProcess();
//     void prepareToPlay(
//         juce::AudioChannelSet outputLayout,
//         double sampleRate,
//         int maximumExpectedSamplesPerBlock
//     );
//     void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);
//     void play(juce::String songPath, double targetSeconds);
//     void pausePlay();
//     bool getIsFullMute() { return isFullMute; }
// };