/**
 * @brief 插件编辑器的宿主窗口（跑在**音频进程**的消息线程上）
 *
 * ── 为什么在音频进程开编辑器 ──────────────────────────────────────────
 *   实验台结论（host-integration.md §8）：插件编辑器必须在有消息循环的线程上创建，
 *   而插件实例本身就在音频进程里。跨进程开编辑器要复制一份实例 + 同步状态，
 *   那是另一套工程（见 docs/audio-routing.md 的讨论）。这里走最直接的路：
 *   音频进程本来就有 `runDispatchLoop()`，直接在本进程开窗。
 *
 * ── 打开时的层级（bringToFrontOnce）────────────────────────────────────
 *   插件窗口是**另一个进程**的顶层窗口，所以它开出来在哪一层取决于插件自己有没有
 *   抢前台：有的调 SetForegroundWindow 就跑到 Flutter 上面，有的只 ShowWindow
 *   就被 Flutter 盖住。
 *
 *   这里要的效果是「**打开时在最上层，之后随便放哪层**」，做法分两步：
 *     1. 打开（或重新显示）时 `setAlwaysOnTop(true)` + `toFront(true)`。
 *        置顶走的是 SetWindowPos(HWND_TOPMOST)，**不需要前台权限**，所以哪怕
 *        音频进程不是前台应用也能真的抬上来（只调 toFront 会被系统降级成闪任务栏）。
 *     2. 等它**第一次失去激活**就 `setAlwaysOnTop(false)`，恢复成普通窗口。
 *        兜底：3 秒内没被激活过（有的编辑器不抢焦点）也降级，免得永久置顶。
 *
 *   已知例外：少数插件自己会 setAlwaysOnTop，它们的窗口本来就一直置顶，
 *   我们的降级会被插件设回去——这种就随它。
 *
 * ── 生命周期（另一个容易踩的地方）──────────────────────────────────────
 *   · 编辑器**只能**在消息线程创建/销毁 → 所有入口都走 `MessageManager::callAsync`；
 *   · 用户点窗口关闭按钮**只隐藏不销毁**：窗口由 GodProcessor 的 map 持有，
 *     再次点「打开编辑器」直接 show 回来，避免"关窗时把自己 delete 掉"的悬垂问题；
 *   · 换链（重建插件实例）之前必须**先**关掉所有编辑器窗口——否则编辑器还拿着
 *     已经析构的 AudioProcessor 指针，下次 repaint 就炸。
 *
 * 用法（只有 GodProcessor 用）：
 *     auto window = std::make_unique<PluginEditorWindow>(processor, title);
 *     if (window->isValid()) mEditorWindows[slot] = std::move(window);
 *     // 已经存在时：
 *     window->bringToFrontOnce();
 */
#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace PluginHost {

    class PluginEditorWindow : public juce::DocumentWindow, private juce::Timer {
    public:
        PluginEditorWindow(juce::AudioProcessor& processor, const juce::String& title);
        ~PluginEditorWindow() override;

        /// 编辑器创建成功了没有（插件可能 hasEditor() == false）
        bool isValid() const noexcept { return mEditor != nullptr; }

        /**
         * @brief 抬到最上层，然后**失去激活就恢复成普通窗口**
         *
         * 打开时、以及"窗口已存在时再点打开编辑器"时都要调。
         */
        void bringToFrontOnce();

        void closeButtonPressed() override;

    private:
        void timerCallback() override;

        juce::AudioProcessor& mProcessor;
        juce::AudioProcessorEditor* mEditor{nullptr};

        /// 是否已经被激活过（用来判断"用户已经看到它了"）
        bool mWasActivated{false};

        /// 计时次数：200ms 一次，超过 15 次（3 秒）还没被激活就兜底降级
        int mTicks{0};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
    };

} // namespace PluginHost
