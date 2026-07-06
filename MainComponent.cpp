#include "MainComponent.h"
#include "Utils/BridgeNames.h"
#include "constants.h"
#include "databaseManage.hpp"
#include "fileManage/fileUtils.hpp"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "serial.hpp"
#include <memory>
#include <vector>

//==============================================================================
MainComponent::MainComponent(){

    //这里定义的都是桥接通信函数
    juce::WebBrowserComponent::Options options;
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled(true)
        .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(UserDirId.getChildFile("UICache"))
        )//windows需要有专门的存储路径，放置应用
        .withNativeFunction(BridgeKeys::inputFiles,//导入文件函数
            [this](
                const juce::Array<juce::var>& args,
                juce::WebBrowserComponent::NativeFunctionCompletion complete
            ){
                juce::MessageManager::callAsync([this,complete](){
                    getMultiMediaFileChoose([complete](const juce::Array<juce::File>& files){

                        if(files.isEmpty()){
                            complete(juce::var());//就算不需要cpp到js的通信也必须发送完成信号
                            return;
                        }//用户取消选择

                        for(auto& file:files){
                            auto song{getStreamMetaData(file)};
                            if(!song.filePath.empty()) SongsManage::getInstance().insertSong(song);
                        }
                        complete(juce::var());
                                
                    },
                        web.get()
                    );
                });
            }
        );
    
    web = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*web);
    #ifdef JUCE_DEBUG
    web->goToURL("http://localhost:5173/");
    #else
        //这里到时候放置release版本的二进制资源打包，因为http://127.0.0.1:5173是开发者专用的
    #endif
}

void MainComponent::resized()
{
    web->setBounds(getLocalBounds());
}

void MainComponent::paint(juce::Graphics& g)
{
    // WebView2 初始化完成前，先用 UI 背景色填充，避免黑屏闪烁
    g.fillAll(juce::Colour(0xfff0f0f0));
}

MainComponent::~MainComponent(){
}
