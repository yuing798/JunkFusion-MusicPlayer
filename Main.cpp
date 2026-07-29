#include "MainComponent.h"
#include "Utils/constants.h"
#include "fileManage/dbManager.hpp"
#include "fileManage/serial.hpp"
#include "fileManage/songsManage.hpp"
#include "image/ImageManager.hpp"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <memory>
#include <stdlib.h>
#include <string>

//==============================================================================
class GuiAppApplication final : public juce::JUCEApplication {
public:
    //==============================================================================
    GuiAppApplication() {}

    // We inject these as compile definitions from the CMakeLists.txt
    // If you've enabled the juce header with `juce_generate_juce_header(<thisTarget>)`
    // you could `#include <JuceHeader.h>` and use `ProjectInfo::projectName` etc. instead.
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return true; }

    //==============================================================================
    void initialise(const juce::String& commandLine) override {
        // This method is where you should put your application's initialisation code..
        juce::ignoreUnused(commandLine);
        // std::cout <<
        // juce::String(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName()).toStdString();

        if (!LocalDirId.exists()) LocalDirId.createDirectory(); // 整个应用的数据文件夹
        if (!imageDirId.exists()) imageDirId.createDirectory(); // 里面放置所有的用户图像信息
        if (!songImageDirId.exists()) songImageDirId.createDirectory(); // 放置歌曲封面信息
        if (!logInfoDirId.exists()) logInfoDirId.createDirectory();     // 日志文件夹

        mLogSystem.init(); // spdlog已经做好了全局唯一单例管理了，不需要自己再做一遍

        dbManager::getInstance();

        // mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override {
        // Add your application's shutdown code here..

        // mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
        juce::ignoreUnused(commandLine);
    }

private:
    logSystem mLogSystem;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION(GuiAppApplication)
