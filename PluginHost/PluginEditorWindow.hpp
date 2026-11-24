/**
 * @brief 插件编辑器的宿主窗口（跑在**音频进程**的消息线程上）
 *
 * ── 为什么在音频进程开编辑器 ──────────────────────────────────────────
 *   实验台结论（host-integration.md §8）：插件编辑器必须在有消息循环的线程上创建，
 *   而插件实例本身就在音频进程里。跨进程开编辑器要复制一份实例 + 同步状态，
 *   那是另一套工程（见 docs/audio-routing.md 的讨论）。这里走最直接的路：
 *   音频进程本来就有 `runDispatchLoop()`，直接在本进程开窗。
 *
 * ── 生命周期（唯一容易踩的地方）────────────────────────────────────────
 *   · 编辑器**只能**在消息线程创建/销毁 → 所有入口都走 `MessageManager::callAsync`；
 *   · 用户点窗口关闭按钮**只隐藏不销毁**：窗口由 GodProcessor 的 map 持有，
 *     再次点「打开编辑器」直接 show 回来，避免"关窗时把自己 delete 掉"的悬垂问题；
 *   · 换链（重建插件实例）之前必须**先**关掉所有编辑器窗口——否则编辑器还拿着
 *     已经析构的 AudioProcessor 指针，下次 repaint 就炸。
 *
 * 用法（只有 GodProcessor 用）：
 *     auto window = std::make_unique<PluginEditorWindow>(processor, title);
 *     if (window->isValid()) mEditorWindows[slot] = std::move(window);
 */
#pragma once

#include "juce_audio_processors/juce_audio_processors.h"
#include "juce_gui_basics/juce_gui_basics.h"

namespace PluginHost {

    class PluginEditorWindow : public juce::DocumentWindow {
    public:
        PluginEditorWindow(juce::AudioProcessor& processor, const juce::String& title);
        ~PluginEditorWindow() override;

        /// 编辑器创建成功了没有（插件可能 hasEditor() == false）
        bool isValid() const noexcept { return mEditor != nullptr; }

        void closeButtonPressed() override;

    private:
        juce::AudioProcessor& mProcessor;
        juce::AudioProcessorEditor* mEditor{nullptr};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
    };

} // namespace PluginHost
