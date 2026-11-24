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
        toFront(true);

        if (auto logger = spdlog::get(LogAudioID))
            logger->info(
                "[PluginEditor] 已打开:{} {}x{}",
                title.toStdString(),
                getWidth(),
                getHeight()
            );
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
        setVisible(false);
    }

} // namespace PluginHost
