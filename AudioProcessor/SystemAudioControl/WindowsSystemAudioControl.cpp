#include "WindowsSystemAudioControl.hpp"

#include <cstddef>
#include <vector>
#include <windows.h>
// #include <winnls.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Media.h>
#include <winrt/base.h>

#include <atomic>
#include <memory>
#include <string>

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Media::Control;

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

static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size =
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

// 平台实现细节全部收敛进 Impl，避免在头文件暴露 WinRT 头
struct WindowsSystemAudioControl::Impl {

    SystemMediaTransportControls smtc{nullptr};
    SystemMediaTransportControlsDisplayUpdater displayUpdater{nullptr};

    std::atomic<bool> initialized{false};

    // 注册的 WinRT 事件令牌，shutdown 时据此反注册
    winrt::event_token playToken{};
    winrt::event_token pauseToken{};
    winrt::event_token nextToken{};
    winrt::event_token previousToken{};
    bool eventsRegistered{false};

    // 获取 SMTC 实例，失败返回 false
    bool resolveSmtc() {
        if (smtc) {
            return true;
        }

        try {
            smtc = SystemMediaTransportControls::GetForCurrentView();
            if (!smtc) {
                return false;
            }
            displayUpdater = smtc.DisplayUpdater();
            return true;
        } catch (...) {
            smtc = nullptr;
            displayUpdater = nullptr;
            return false;
        }
    }
};

WindowsSystemAudioControl::WindowsSystemAudioControl() : mImpl(std::make_unique<Impl>()) {}

WindowsSystemAudioControl::~WindowsSystemAudioControl() { shutdown(); }

bool WindowsSystemAudioControl::initialize() {
    if (mImpl->initialized.load()) {
        return true;
    }

    if (!mImpl->resolveSmtc()) {
        // TODO(连接): 此处需要调用方提供一个日志回调/工具，失败时打印错误日志
        return false;
    }

    try {
        mImpl->smtc.IsEnabled(true);
        mImpl->smtc.IsPlayEnabled(true);
        mImpl->smtc.IsPauseEnabled(true);
        mImpl->smtc.IsNextEnabled(true);
        mImpl->smtc.IsPreviousEnabled(true);
    } catch (...) {
        // TODO(连接): 打印初始化异常日志
        return false;
    }

    mImpl->smtc.PlaybackStatus(MediaPlaybackStatus::Closed);

    // 注册媒体按键事件；回调全部切回主线程触发（WinRT 事件线程不能直接碰业务/UI 状态）
    auto onPlayEvt = [this](auto&&, auto&& e) {
        if (e.Button() == SystemMediaTransportControlsButton::Play && onPlay) {
            onPlay();
        }
    };
    auto onPauseEvt = [this](auto&&, auto&& e) {
        if (e.Button() == SystemMediaTransportControlsButton::Pause && onPause) {
            onPause();
        }
    };
    auto onNextEvt = [this](auto&&, auto&& e) {
        if (e.Button() == SystemMediaTransportControlsButton::Next && onNext) {
            onNext();
        }
    };
    auto onPrevEvt = [this](auto&&, auto&& e) {
        if (e.Button() == SystemMediaTransportControlsButton::Previous && onPrevious) {
            onPrevious();
        }
    };
    // FIXME(连接): 系统媒体按键无法区分“快进”与“快退”，onSeek 的方向在此处理不了。
    //             若需要 seek，请改用 TimelineProperties 的 PositionChangeRequested 事件，
    //             或自行扩展基类接口增加 onSeekForward / onSeekBackward 回调。
    mImpl->playToken = mImpl->smtc.ButtonPressed(onPlayEvt);
    mImpl->pauseToken = mImpl->smtc.ButtonPressed(onPauseEvt);
    mImpl->nextToken = mImpl->smtc.ButtonPressed(onNextEvt);
    mImpl->previousToken = mImpl->smtc.ButtonPressed(onPrevEvt);

    mImpl->eventsRegistered = true;
    mImpl->initialized.store(true);
    return true;
}

void WindowsSystemAudioControl::updateMetadata(const MediaMetadata& metadata) {
    if (!mImpl->initialized.load() || !mImpl->smtc) {
        return;
    }

    try {
        mImpl->displayUpdater.Type(MediaPlaybackType::Music);
        mImpl->displayUpdater.MusicProperties().Title(Utf8ToWide(metadata.title));
        mImpl->displayUpdater.MusicProperties().Artist(
            Utf8ToWide(joinIntoString(metadata.artists, " / "))
        );
        mImpl->displayUpdater.Update();
    } catch (...) {
        // TODO(连接): 打印元数据更新失败日志
    }
}

void WindowsSystemAudioControl::updatePlaybackState(PlaybackState state) {
    if (!mImpl->initialized.load() || !mImpl->smtc) {
        return;
    }

    try {
        mImpl->smtc.PlaybackStatus(
            state == PlaybackState::Play ? MediaPlaybackStatus::Playing
                                         : MediaPlaybackStatus::Paused
        );
    } catch (...) {
        // TODO(连接): 打印播放状态更新失败日志
    }
}

void WindowsSystemAudioControl::shutdown() {
    if (!mImpl->initialized.exchange(false)) {
        return;
    }

    try {
        if (mImpl->smtc && mImpl->eventsRegistered) {
            mImpl->smtc.ButtonPressed(mImpl->playToken);
            mImpl->smtc.ButtonPressed(mImpl->pauseToken);
            mImpl->smtc.ButtonPressed(mImpl->nextToken);
            mImpl->smtc.ButtonPressed(mImpl->previousToken);
        }
        if (mImpl->smtc) {
            mImpl->smtc.IsEnabled(false);
        }
    } catch (...) {
        // TODO(连接): 打印释放异常日志
    }

    mImpl->eventsRegistered = false;
    mImpl->smtc = nullptr;
    mImpl->displayUpdater = nullptr;
}
