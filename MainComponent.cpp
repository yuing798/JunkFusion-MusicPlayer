#include "MainComponent.h"
#include "Utils/BridgeNames.h"
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
        .withNativeFunction(BridgeKeys::inputFiles,//导入文件函数
            [this](
                const juce::Array<juce::var>& args,
                juce::WebBrowserComponent::NativeFunctionCompletion complete
            ){
                DBG(">>> inputFiles native function called, args=" << args.size());
                juce::MessageManager::callAsync([this, complete, args](){
                DBG(">>> inputFiles on message thread, opening file chooser");
                getMultiMediaFileChoose([complete,args](const juce::Array<juce::File>& files){

                    juce::MessageManager::callAsync(
                        [complete, files, args](){
                            if(files.isEmpty()){
                                complete(juce::var());
                                return;
                            }//用户取消选择

                            std::vector<SongInfo> songList;
                            for(auto& file:files){
                                auto song{getStreamMetaData(file)};
                                if(!song.filePath.empty()) SongsManage::getInstance().insertSong(song);
                            }

                            //返回给js的结果
                            juce::Array<juce::var> results;
                            for(auto& song:songList){
                                results.add(SongInfo::toVar(song));
                            }
                            complete(juce::var(results));
                            
                        }
                    );
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
        //这里到时候放置release版本的goToURL，因为http://127.0.0.1:5173是开发者专用的
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
