#include "PluginHost/PluginEditorWindow.hpp"

#include "Utils/constants.h"

#include <spdlog/spdlog.h>

namespace PluginHost {

    PluginEditorWindow::PluginEditorWindow(
        juce::AudioProcessor& processor,
        const juce::String& title
    )
        : juce::DocumentWindow(title, juce::Colours::darkgrey, DocumentWindow::closeButton),
          mProcessor(processor) {
        setUsingNativeTitleBar(true);
        setResizable(false, false);

        // 必须在消息线程调（所有入口都走 MessageManager::callAsync）
        mEditor = processor.createEditorIfNeeded();
        if (mEditor == nullptr) {
            if (auto logger = spdlog::get(LogAudioID))
                logger->warn("[PluginEditor] 插件没有编辑器:{}", title.toStdString());
            setVisible(false);
            return;
        }

        setContentOwned(mEditor, true);
        setResizable(mEditor->isResizable(), false);
        setSize(mEditor->getWidth(), mEditor->getHeight());
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
        // 打开时抬到最上层；失去激活后会自动恢复成普通窗口（见 bringToFrontOnce）
        bringToFrontOnce();

        if (auto logger = spdlog::get(LogAudioID))
            logger->info(
                "[PluginEditor] 已打开:{} {}x{}",
                title.toStdString(),
                getWidth(),
                getHeight()
            );
    }

    void PluginEditorWindow::bringToFrontOnce() {
        // 置顶走 SetWindowPos(HWND_TOPMOST)，不需要前台权限：
        // 只调 toFront() 的话，音频进程不是前台应用时会被系统降级成"闪任务栏"，
        // 窗口其实还压在 Flutter 下面。
        setAlwaysOnTop(true);
        toFront(true);

        mWasActivated = false;
        mTicks = 0;
        startTimer(200);
    }

    void PluginEditorWindow::timerCallback() {
        if (isActiveWindow()) {
            // 用户已经点到它了，说明"露过脸"了
            mWasActivated = true;
            return;
        }

        // 露过脸 + 现在失去激活 -> 恢复成普通窗口，之后爱放哪层放哪层。
        // 兜底：3 秒（15 × 200ms）都没被激活过，也降级 —— 有的编辑器不抢焦点，
        // 不兜底的话它会永久置顶，比"被盖住"还烦。
        if (mWasActivated || ++mTicks > 15) {
            setAlwaysOnTop(false);
            stopTimer();
        }
    }

    PluginEditorWindow::~PluginEditorWindow() {
        // 顺序很关键：先告诉插件"编辑器要没了"（有些插件要用它断开监听），
        // 再让 DocumentWindow 删掉 content
        if (mEditor != nullptr) {
            mProcessor.editorBeingDeleted(mEditor);
            mEditor = nullptr;
        }
        clearContentComponent();
    }

    void PluginEditorWindow::closeButtonPressed() {
        // 只隐藏，不销毁：窗口的拥有者是 GodProcessor 的 map，
        // 在这里 delete this 会让那个 unique_ptr 变悬垂。
        // 再点一次「打开编辑器」会 setVisible(true) 回来。
        // 顺手撤掉置顶：下次重新打开时 bringToFrontOnce() 会重新抬一次。
        setAlwaysOnTop(false);
        stopTimer();
        setVisible(false);
    }

} // namespace PluginHost
