#pragma once

#include <functional>

#include "juce_core/juce_core.h"
class SystemAudioControl {
public:
    virtual ~SystemAudioControl() = default;

    struct MediaMetadata {
        juce::String title;
        juce::StringArray artists;
    };

    enum class PlaybackState { Play, Pause };

    virtual bool initialize() = 0;                         // 注册系统媒体会话
    virtual void updateMetadata(const MediaMetadata&) = 0; // 标题/艺术家（可选封面）
    virtual void updatePlaybackState(PlaybackState) = 0;   // 播放/暂停/停止
    virtual void shutdown() = 0;                           // 释放

    // 回调：系统按键
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onNext;
    std::function<void()> onPrevious;
    std::function<void(double)> onSeek;
};