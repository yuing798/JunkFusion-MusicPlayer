#pragma once

#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include <atomic>
#include <functional>

// 播放控制放在整个DSP链路的最后一环
class PlayPauseControl {

private:
    SmoothLinearValue smoothPlayPause;
    std::atomic_bool isFullMute{false};
    std::atomic_bool hadPlayStatle{false}; // 播放是否进入平稳态

public:
    enum class PlayPauseState { Play, Pause };
    enum class BeginEndState { Begin, End, Nor };

    PlayPauseControl() {}

    /**
     * @brief Set the Playback State object 开始播放或者开始暂停
     *
     * @param playOrPause 播放或者暂停状态
     * @param beginOrEnd 在歌曲开头或者末尾执行播放暂停无须淡入淡出
     */
    void setPlaybackState(PlayPauseState playOrPause, BeginEndState beginOrEnd);
    std::function<void(void)> onPlayStable; // 播放进入稳定期
    std::function<void(void)> onPauseOver;  // 暂停结束，完全静音
    void prepareToPlay(float sampleRate);
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);

    bool getIsFullMute() noexcept { return isFullMute; }

    DONT_COPY_AND_MOVE(PlayPauseControl)
};