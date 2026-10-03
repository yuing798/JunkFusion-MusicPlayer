#pragma once

#include <functional>
#include <string>
#include <vector>

class SystemAudioControl {

protected:
    static constexpr const int fastForwardAndRewindTime = 10; // 每次快进和快退的时间

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

public:
    virtual ~SystemAudioControl() = default;

    struct MediaMetadata {
        std::string title;
        std::vector<std::string> artists;
    };

    enum class PlaybackState { Play, Pause };
    enum class LogRank { Debug, Info, Error };

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
    std::function<void(LogRank, std::string)> onLog;
    std::function<void(int)> onFastForward; // 快进
    std::function<void(int)> onRewind;      // 快退
};