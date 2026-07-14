#include "MainComponent.h"
#include "Utils/constants.h"
#include "fileManage/songsManage.hpp"
#include "fileManage/serial.hpp"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <memory>
#include <stdlib.h>
#include <string>
#include "fileManage/dbManager.hpp"
#include "image/ImageManager.hpp"

//==============================================================================
class GuiAppApplication final : public juce::JUCEApplication
{
public:
    //==============================================================================
    GuiAppApplication() {}

    // We inject these as compile definitions from the CMakeLists.txt
    // If you've enabled the juce header with `juce_generate_juce_header(<thisTarget>)`
    // you could `#include <JuceHeader.h>` and use `ProjectInfo::projectName` etc. instead.
    const juce::String getApplicationName() override       { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override    { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    //==============================================================================
    void initialise (const juce::String& commandLine) override
    {
        // This method is where you should put your application's initialisation code..
        juce::ignoreUnused (commandLine);
        // std::cout << juce::String(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName()).toStdString();

        #ifdef JUCE_DEBUG
        _putenv_s("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS", "--auto-open-devtools-for-tabs");//这一行的作用是打开webview2的控制台
        #endif

        if(!LocalDirId.exists()) LocalDirId.createDirectory();//整个应用的数据文件夹
        if(!imageDirId.exists()) imageDirId.createDirectory();//里面放置所有的用户图像信息
        if(!songImageDirId.exists()) songImageDirId.createDirectory();//放置歌曲封面信息
        if(!logInfoDirId.exists()) logInfoDirId.createDirectory();//日志文件夹

        mLogSystem.init();//spdlog已经做好了全局唯一单例管理了，不需要自己再做一遍
        ImageManager::getInstance();//将应用级图片加载到内存中
        
        dbManager::getInstance();

        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        // Add your application's shutdown code here..

        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const juce::String& commandLine) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
        juce::ignoreUnused (commandLine);
    }

    //==============================================================================
    /*
        This class implements the desktop window that contains an instance of
        our MainComponent class.
    */
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (name,
                              juce::Colour(0xfff0f0f0),  // 与 web UI --colorMain 一致，避免 WebView2 加载前的黑屏闪烁
                              allButtons)
        {
            setUsingNativeTitleBar (true);

            // 先全屏，再创建内容组件。
            // 否则 WebView2 会先以默认窗口大小初始化，setFullScreen 放大后
            // WebView2 来不及跟上，产生"黑屏 + 左上角白块"的闪烁。
            setFullScreen (true);
            setResizable (true, true);

            // resizeToContent = false，因为窗口已经是全屏，不需要再根据内容调整大小
            setContentOwned (new MainComponent(), false);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            // This is called when the user tries to close this window. Here, we'll just
            // ask the app to quit when this happens, but you can change this to do
            // whatever you need.
            getInstance()->systemRequestedQuit();
        }

        /* Note: Be careful if you override any DocumentWindow methods - the base
           class uses a lot of them, so by overriding you might break its functionality.
           It's best to do all your work in your content component instead, but if
           you really have to override any DocumentWindow methods, make sure your
           subclass also calls the superclass's method.
        */

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    logSystem mLogSystem;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (GuiAppApplication)
