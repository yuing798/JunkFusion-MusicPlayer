#pragma once

#include <functional>
#include <string>
#include <vector>

class SystemAudioControl {
public:
    virtual ~SystemAudioControl() = default;

    struct MediaMetadata {
        std::string title;
        std::vector<std::string> artists;
    };

    static std::string
    joinIntoString(const std::vector<std::string>& stringArray, std::string separtor) {
        std::string finalS;
        for (size_t i = 0; i < stringArray.size(); i++) {
            finalS += stringArray[i];
            if (i < stringArray.size() - 1) {
                finalS += separtor;
            }
        }
        return finalS;
    }

    enum class PlaybackState { Play, Pause };

    virtual bool initialize() = 0;                         // 注册系统媒体会话
    virtual void updateMetadata(const MediaMetadata&) = 0; // 标题/艺术家（可选封面）
    virtual void updatePlaybackState(PlaybackState) = 0;   // 播放/暂停/停止
    virtual void shutdown() = 0;                           // 释放

    enum class LogRank { Debug, Info, Error };

    // 回调：系统按键
    std::function<void()> onPlay;
    std::function<void()> onPause;
    std::function<void()> onNext;
    std::function<void()> onPrevious;
    std::function<void(double)> onSeek;
    std::function<void(LogRank, std::string)> onLog;
    std::function<void()> onFastForward; // 快进
    std::function<void()> onRewind;      // 快退
};