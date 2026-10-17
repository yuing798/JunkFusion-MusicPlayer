#pragma once

#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include <array>
#include <atomic>
#include <functional>
class CrossFadeArea {
private:
    std::atomic<int> currentCrossFadeIndex{0}; // 交叉淡化区的查表索引
    std::atomic<bool> isCrossFade;             // 是否在交叉淡化区
    std::atomic_bool mIsSongChange;
    int mainDuckIndex{0}; // 主甲板号
    float mSampleRate;

    const int songChangeMs{600};
    const int seekMs{50}; // 歌曲切换的交叉淡化区为600ms，进度条跳转为50ms
public:
    CrossFadeArea();
    void prepareToPlay(float sampleRate);
    void processBlock(
        std::array<juce::AudioBuffer<float>*, 2> inputs,
        juce::AudioBuffer<float>& output,
        int numSamples
    );

    /**
     * @brief Set the Cross Fade Begin object,交叉淡化开始
     *
     * @param isSongChange 是否为切换歌曲，因为歌曲切换和进度条跳转的交叉淡化长度不同
     */
    void setCrossFadeBegin(bool isSongChange);
    std::function<void(void)> onCrossFadeEnd; // 交叉淡化区域结束

    int getMainDuckIndex() noexcept { return mainDuckIndex; }
};