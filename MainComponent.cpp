#include "MainComponent.h"
#include "Utils/BridgeNames.h"
#include "Utils/otherUtils.hpp"
#include "constants.h"
#include "juce_graphics/juce_graphics.h"
#include "songsManage.hpp"
#include "dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "serial.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include "fileManage/dbManager.hpp"

//==============================================================================
MainComponent::MainComponent(){

    //这里定义的都是桥接通信函数
    juce::WebBrowserComponent::Options options;
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled(true)
        .withWinWebView2Options(juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder(LocalDirId)
        )//windows需要有专门的存储路径，放置应用web缓存
        
        //下面放置的是后端需要直接和前端交互的函数
        .withResourceProvider([this](const juce::String& path) -> std::optional<juce::WebBrowserComponent::Resource> {

            juce::StringArray tokens;
            tokens.addTokens(path,"/","");//将原始URL按照斜杠进行切分
            if(tokens[0] == "songId"){
                int64_t songId{tokens[1].getLargeIntValue()};
                if(tokens[2] == "image"){//歌曲的信息,URL格式为/songId/8175019024(id号)/image/imageType
                    std::string hash{dbManager::getInstance().getSongsManager().getImageHashBySongId(songId)};
                    juce::File songImageDir{songImageDirId.getChildFile(hash)};

                    if(tokens[3] == "50x50"){
                        if(hash.empty()){
                            return juce::WebBrowserComponent::Resource(
                                imageHolder50x50,
                                "image/png"
                            );//没有图片的话返回占位图片
                        }else{
                            
                            juce::File image50x50{songImageDir.getChildFile("50x50.jpg")};
                            if(!image50x50.existsAsFile()){//说明是第一次加载50x50图片，没有加入缓存
                                juce::Image originalImage{juce::ImageCache::getFromFile(
                                    songImageDir.getChildFile("original.jpg")
                                )};//得到原始图片

                                //裁剪图片
                                juce::Image songImage50x50 = ImageManager::clipMode(originalImage,50,50);

                                //加载到磁盘和内存中
                                auto vec = ImageManager::jpg2MemoryAndFile(songImage50x50,70,image50x50);
                                return juce::WebBrowserComponent::Resource(vec,"image/jpeg");
                            }else{
                                return juce::WebBrowserComponent::Resource(
                                    loadFile2ByteVector(image50x50),
                                    "image/jpeg"
                                );
                            }
                        }
                    }
                }
            }
            return std::nullopt;
        }
            #ifdef JUCE_DEBUG
            ,"http://localhost:5173"//debug模式下需要调用外部域名
            #endif
        )
        .withOptionsFrom(mSongsManagerBuilder)

        //withNativeFunction这个逼函数默认运行在Message Thread
        .withNativeFunction(B_inputFiles::name,//导入文件函数,导入文件函数因为需要绑定模态窗所以放在MainComponent中比较合适
            [this](
                const juce::Array<juce::var>& args,
                juce::WebBrowserComponent::NativeFunctionCompletion complete
            ){

                getMultiMediaFileChoose([complete](const juce::Array<juce::File>& files){

                    auto logger{spdlog::get(LogSchedulerID)};
                    logger->info("开始导入音频文件");

                    if(files.isEmpty()){
                        logger->info("用户取消了音频文件导入");
                        complete(juce::var());//就算不需要cpp到js的通信也必须发送完成信号
                        return;
                    }//用户取消选择

                    dbManager::getInstance().runOnWrite([files,complete = std::move(complete)](){
                        juce::String errorStr{""};
                        const int numAll{files.size()};
                        int num4ErrorFile{0};
                        for(auto& file:files){
                            bool isSuccess = dbManager::getInstance().getSongsManager().insertSong(file);
                            if(!isSuccess){
                                errorStr += (file.getFileName()) + utf8("\n"); 
                                num4ErrorFile++;
                            }
                        }
                        if(errorStr.isNotEmpty()){
                            auto obj{new juce::DynamicObject()};
                            obj->setProperty(
                                B_event::partError,
                                utf8("导入完成\n成功 ") + 
                                juce::String(numAll - num4ErrorFile) 
                                + utf8("首----失败") 
                                + juce::String(num4ErrorFile) 
                                + utf8("首:\n")
                                + errorStr 
                            );
                            complete(juce::var(obj));
                            return;
                        }else{
                            auto obj{new juce::DynamicObject()};
                            obj->setProperty(
                                B_event::fullSuccess,
                                utf8("导入成功\n共导入") + juce::String(numAll) + utf8("首"));
                            complete(juce::var(obj));
                            return;
                        }
                    });

                },
                    web.get()
                );

            }
        );
    
    web = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*web);
    #ifdef JUCE_DEBUG
    web->goToURL("http://localhost:5173/");
    #else
    web->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    //这里到时候放置release版本的二进制资源打包，因为http://127.0.0.1:5173是开发者专用的
    #endif
}

void MainComponent::resized()
{
    web->setBounds(getLocalBounds());
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xfff0f0f0));
}

MainComponent::~MainComponent(){
}
