#include "WindowsSystemAudioControl.hpp"
#include "SystemAudioControl.hpp"

#include <cstddef>
#include <vector>
#include <windows.h>
// #include <winnls.h>
#include <shobjidl_core.h>
#include <systemmediatransportcontrolsinterop.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Media.Playback.h>
#include <winrt/Windows.Media.h>
#include <winrt/base.h>

#include <atomic>
#include <memory>
#include <string>
#include <winrt/impl/Windows.Media.0.h>

using namespace winrt::Windows::Media;
using namespace winrt::Windows::Media::Control;

static std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size =
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), size);
    return wide;
}

// 静态函数，注册一个只属于自己的窗口类
static const wchar_t* kSmtcWindowClassName = L"JunkFusion_SMTC_HiddenWindow";

static bool registerSmtcWindowClass() {
    static bool registered = false;
    if (registered) return true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW; // 用系统默认消息处理即可，我们不处理任何消息
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kSmtcWindowClassName;

    if (!RegisterClassExW(&wc)) {
        DWORD err = GetLastError();
        // ERROR_CLASS_ALREADY_EXISTS (1410) 表示类已注册，可视为成功
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }
    }
    registered = true;
    return true;
}

static HWND createSmtcHiddenWindow() {
    if (!registerSmtcWindowClass()) return nullptr;

    // WS_POPUP + 0 尺寸 + 不显示
    // WS_EX_TOOLWINDOW 让它在任务栏里不出现，即使意外显示也不会留下痕迹
    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, // 不出现在任务栏、不抢焦点
        kSmtcWindowClassName,
        L"SMTC Hidden",
        WS_POPUP, // 无边框弹出式
        0,
        0,
        0,
        0, // 位置和尺寸都是 0
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );
    return hwnd; // 注意：不要 ShowWindow，保持隐藏
}

// 平台实现细节全部收敛进 Impl，避免在头文件暴露 WinRT 头
struct WindowsSystemAudioControl::Impl {
    HWND hiddenHwnd{nullptr}; // smtc要求进程必须有属于自己的窗口，这里使用隐藏窗口实现
    SystemMediaTransportControls smtc{nullptr};
    SystemMediaTransportControlsDisplayUpdater displayUpdater{nullptr};

    std::atomic<bool> initialized{false};

    // 注册的 WinRT 事件令牌，shutdown 时据此反注册
    winrt::event_token playToken{};
    winrt::event_token pauseToken{};
    winrt::event_token nextToken{};
    winrt::event_token previousToken{};
    winrt::event_token fastForwardToken{};
    winrt::event_token rewindToken{};
    // winrt::event_token stopToken{};
    bool eventsRegistered{false};

    // 获取 SMTC 实例，失败返回 false
    bool resolveSmtc() {
        if (smtc) {
            return true;
        }
        if (!hiddenHwnd) {
            hiddenHwnd = createSmtcHiddenWindow();
            if (!hiddenHwnd) return false;
        }

        try {
            auto interop = winrt::get_activation_factory<
                SystemMediaTransportControls,
                ISystemMediaTransportControlsInterop>();

            winrt::check_hresult(interop->GetForWindow(
                hiddenHwnd, // 用自己的窗口，同进程，合法
                winrt::guid_of<SystemMediaTransportControls>(),
                winrt::put_abi(smtc)
            ));

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
    if (onLog) onLog(LogRank::Debug, "开始初始化smtc");
    if (mImpl->initialized.load()) {
        return true;
    }

    if (!mImpl->resolveSmtc()) {
        if (onLog) onLog(LogRank::Error, "解析STMC失败");
        return false;
    }

    try {
        mImpl->smtc.IsEnabled(true);
        mImpl->smtc.IsPlayEnabled(true);
        mImpl->smtc.IsPauseEnabled(true);
        mImpl->smtc.IsNextEnabled(true);
        mImpl->smtc.IsPreviousEnabled(true);
        mImpl->smtc.IsFastForwardEnabled(true);
        mImpl->smtc.IsRewindEnabled(true);
        // mImpl->smtc.IsStopEnabled(true);
    } catch (...) {
        if (onLog) onLog(LogRank::Error, "windows系统音频控制初始化失败");
        return false;
    }

    mImpl->smtc.PlaybackStatus(MediaPlaybackStatus::Closed);

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
    auto onFastForwardEvt = [this](auto&&, auto&& e) {
        if (e.Button() == SystemMediaTransportControlsButton::FastForward && onFastForward) {
            onFastForward();
        }
    };
    auto onRewindEvt = [this](auto&&, auto&& e) {
        if (e.Button() == SystemMediaTransportControlsButton::Rewind && onRewind) {
            onRewind();
        }
    };
    // auto onStopEvt = [this](auto&&, auto&& e) {
    //     if (e.Button() == SystemMediaTransportControlsButton::Stop && onStop) {
    //         onStop();
    //     }
    // };

    // FIXME(连接): 系统媒体按键无法区分“快进”与“快退”，onSeek 的方向在此处理不了。
    //             若需要 seek，请改用 TimelineProperties 的 PositionChangeRequested 事件，
    //             或自行扩展基类接口增加 onSeekForward / onSeekBackward 回调。
    mImpl->playToken = mImpl->smtc.ButtonPressed(onPlayEvt);
    mImpl->pauseToken = mImpl->smtc.ButtonPressed(onPauseEvt);
    mImpl->nextToken = mImpl->smtc.ButtonPressed(onNextEvt);
    mImpl->previousToken = mImpl->smtc.ButtonPressed(onPrevEvt);
    mImpl->fastForwardToken = mImpl->smtc.ButtonPressed(onFastForwardEvt);
    mImpl->rewindToken = mImpl->smtc.ButtonPressed(onRewindEvt);

    mImpl->eventsRegistered = true;
    mImpl->initialized.store(true);
    return true;
}

void WindowsSystemAudioControl::updateMetadata(const MediaMetadata& metadata) {
    std::string logStr{"准备更新smtc元数据:标题:"};
    logStr += metadata.title;
    logStr += ",艺术家数组:";
    logStr += SystemAudioControl::joinIntoString(metadata.artists, " / ");
    if (onLog) onLog(LogRank::Debug, logStr);

    if (!mImpl->initialized.load() || !mImpl->smtc) {
        return;
    }

    try {
        mImpl->displayUpdater.Type(MediaPlaybackType::Music);
        mImpl->displayUpdater.MusicProperties().Title(Utf8ToWide(metadata.title));
        mImpl->displayUpdater.MusicProperties().Artist(
            Utf8ToWide(SystemAudioControl::joinIntoString(metadata.artists, " / "))
        );
        mImpl->displayUpdater.Update();
    } catch (...) {
        if (onLog) onLog(LogRank::Error, "更新元数据失败");
    }
}

void WindowsSystemAudioControl::updatePlaybackState(PlaybackState state) {
    if (!mImpl->initialized.load() || !mImpl->smtc) {
        if (onLog) onLog(LogRank::Debug, "smtc播放状态发生未知原因提前return");
        return;
    }

    try {
        mImpl->smtc.PlaybackStatus(
            state == PlaybackState::Play ? MediaPlaybackStatus::Playing
                                         : MediaPlaybackStatus::Paused
        );
        std::string logState{state == PlaybackState::Play ? "播放" : "暂停"};
        if (onLog) onLog(LogRank::Debug, "smtc播放状态已更新:当前状态:" + logState);
    } catch (...) {
        if (onLog) onLog(LogRank::Error, "更新播放状态失败");
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
            mImpl->smtc.ButtonPressed(mImpl->fastForwardToken);
            mImpl->smtc.ButtonPressed(mImpl->rewindToken);
            // mImpl->smtc.ButtonPressed(mImpl->stopToken);
        }
        if (mImpl->smtc) {
            mImpl->smtc.IsEnabled(false);
        }

        mImpl->smtc = nullptr;
        mImpl->displayUpdater = nullptr;

        // 2. 再销毁隐藏窗口（顺序很重要：SMTC 释放之后再销毁）
        if (mImpl->hiddenHwnd) {
            DestroyWindow(mImpl->hiddenHwnd);
            mImpl->hiddenHwnd = nullptr;
        }

    } catch (...) {
        if (onLog) onLog(LogRank::Error, "windows系统音频控制释放资源异常");
    }

    mImpl->eventsRegistered = false;
    mImpl->smtc = nullptr;
    mImpl->displayUpdater = nullptr;
}
