#include "MainComponent.h"
#include "databaseManage.hpp"
#include "fileManage/fileUtils.hpp"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "serial.hpp"
#include <memory>
#include <vector>

//这里定义所有的桥接函数ID
static constexpr const char* fileInputId{"fileInput"};//音频文件导入应用
static constexpr const char* dirScanId{"dirScan"};//文件夹扫描

//==============================================================================
MainComponent::MainComponent(){

    //这里定义的都是桥接通信函数
    juce::WebBrowserComponent::Options options;
    options = options
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled(true)
        .withNativeFunction(
            fileInputId,
            [this](
                const juce::Array<juce::var>& args,
                juce::WebBrowserComponent::NativeFunctionCompletion complete
            ){
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
                            int page{0};
                            bool ascending{true};
                            SongsManage::SortMode mode{SongsManage::SortMode::ByAddTime};
                            if(args.size()>=3){//边界检查
                                page = (int)args[0];
                                ascending = (bool)args[1];
                                switch ((int)args[2]){
                                    case 0:
                                        mode = SongsManage::SortMode::ByAddTime;
                                        break;
                                    case 1:
                                        mode = SongsManage::SortMode::ByName;
                                        break;
                                    case 2:
                                        mode = SongsManage::SortMode::ByPlayTimes;
                                        break;
                                }
                            }

                            songList = SongsManage::getInstance().getSongPage(
                                page,
                                20,//每一页20首歌曲，这是不变的
                                ascending,
                                mode
                            );//导入文件的时候直接做全量取出重排即可

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
            }
        );
    
    web = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*web);
    #ifdef JUCE_DEBUG
    web->goToURL("http://127.0.0.1:5173");
    #else
        //这里到时候放置release版本的goToURL，因为http://127.0.0.1:5173是开发者专用的
    #endif
}

void MainComponent::resized()
{
    web->setBounds(getLocalBounds());
}

MainComponent::~MainComponent(){
}
