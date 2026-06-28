#include "allMusicPage.hpp"
#include "BinaryData.h"
#include "FontAbout/font.h"
#include "buttons.hpp"
#include "databaseManage.hpp"
#include "fileMessage.hpp"
#include "fileUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "popupWindow.hpp"
#include "serial.hpp"
#include <memory>
#include <string>
#include <utility>

PageChange::PageChange(){
    for(auto& button : Buttons){
        button.setSize(30, 30);
        button.addListener(this);
        addAndMakeVisible(button);
    }
    popButton.addListener(this);
    previousButton.addListener(this);
    nextButton.addListener(this);
    addAndMakeVisible(popButton);
    addAndMakeVisible(previousButton);
    addAndMakeVisible(nextButton);

    mTextEditor.addListener(this);
    addAndMakeVisible(mTextEditor);

    for(auto& ellipsisLabel :ellipsisLabels){
        addAndMakeVisible(ellipsisLabel);
        ellipsisLabel.setSize(30, 30);
    }

}

void PageChange::buttonClicked (Button* button){
    
}
void PageChange::resized(){
    auto local{getLocalBounds()};
    auto height{local.getHeight()};
    auto row1{local.removeFromTop(height/2.0f).reduced(5)};
    auto row2{local.reduced(5)};
    auto  rowHeight{height/2.0f-10};
    previousButton.setBounds(row1.removeFromLeft(row1.getHeight()).reduced(5));
    nextButton.setBounds(row1.removeFromRight(row1.getHeight()).reduced(5));

    if(nowPage == 1){
        previousButton.setEnabled(false);
    }else if(nowPage == numPages){
        nextButton.setEnabled(false);
    }else{
        previousButton.setEnabled(true);
        nextButton.setEnabled(true);
    }

    if(numPages <= 7){
        for(auto& ellipsisLabel :ellipsisLabels){
            ellipsisLabel.setVisible(false);
        }
    }

}
void PageChange::textEditorReturnKeyPressed(juce::TextEditor& editor){
    if(&editor == &mTextEditor){
        auto targetPage{std::stoi(editor.getText().toStdString())};
    }
}
PageChange::~PageChange(){
    mTextEditor.removeListener(this);
    for(auto& button : Buttons){
        button.removeListener(this);
    }
    popButton.removeListener(this);
    previousButton.removeListener(this);
    nextButton.removeListener(this);
}

AllMusicComponent::AllMusicComponent(){

}

allMusicPage::allMusicPage(){
    addAndMakeVisible(refreshButton);
    addAndMakeVisible(selectFileButton);
    addAndMakeVisible(allMusicLabel);
    addAndMakeVisible(mViewPort);
    addAndMakeVisible(numSongsLabel);
    addAndMakeVisible(mUpDownButton);

    selectFileButton.addListener(this);
    seqWays.addItem(U("歌曲名称排列"));
    seqWays.addItem(U("添加时间排列"));
    addAndMakeVisible(seqWays);

    int comboSelectedIndex{0};
    if(Serial::getInstance().getUIRoot().isValid()){
        comboSelectedIndex = Serial::getInstance().getUIRoot().getProperty(SERIAL_allMusicSeqWays,0);
    }
    seqWays.setSelectedItemIndex(comboSelectedIndex);

    seqWays.onItemSelected([this](int value){
        //更新排序方法
        //refreshPage(int page);//传参为当前的页码数
        //加入序列化
        Serial::getInstance().getUIRoot().setProperty(SERIAL_allMusicSeqWays, value, nullptr);
    });
}
void allMusicPage::resized(){

    auto local = getLocalBounds();
    auto height = local.getHeight();
    mViewPort.setBounds(local.removeFromBottom(height * 0.8f));
    auto row2 = local.removeFromBottom(height * 0.09f).reduced(5.0f);
    auto row1 = local.reduced(5);

    row2.removeFromLeft(20);
    selectFileButton.setBounds(row2.removeFromLeft(80).reduced(5,10));
    refreshButton.setBounds(row2.removeFromLeft(row2.getHeight()).reduced(10));
    row2.removeFromRight(30);
    mUpDownButton.setBounds(row2.removeFromRight(row2.getHeight()).reduced(10));
    seqWays.setBounds(local.removeFromRight(120).reduced(10));

    allMusicLabel.setBounds(row1.removeFromLeft(90));
    numSongsLabel.setBounds(row1.removeFromLeft(90));

}
void allMusicPage::buttonClicked(juce::Button* button)
{
    if (button == &selectFileButton)
    {
        selectFileButton.setClickingTogglesState(false);//在推入数据库的时候先把按钮锁定
        getMultiMediaFileChoose(
            [](const juce::Array<juce::File>& selectedFiles)
            {
                for (auto& file : selectedFiles)
                {
                    auto eachSong{getStreamMetaData(file)};//FFmpeg提取原数据
                    if(!eachSong.filePath.empty()) SongsManage::getInstance().insertSong(eachSong);
                    //推入数据库
                }
            },
            this
        );
        
        //使用FFmpeg提取元数据
        //推入数据库

        //推入数据库后就可以清空了，等待下一次推入
        selectFileButton.setClickingTogglesState(true);
    }
}
// void allMusicPage::paint(juce::Graphics& g){

// }