#include "allMusicPage.hpp"
#include "FontAbout/font.h"
#include "databaseManage.hpp"
#include "fileUtils.hpp"
#include "juce_graphics/juce_graphics.h"

EachSong::EachSong(int          songNo,
    const juce::String& songName,
    const juce::String& songComposer,
    const juce::String& songAlbum,
    const juce::String& songStyle,
    const juce::String& playNums,
    const juce::String& songLength,
    const juce::String& songBitRate)
: No_(juce::String(songNo))
, name(songName)
, composer(songComposer)
, album(songAlbum)
, style(songStyle)
, mWhatsMore(U("播放次数: ") + playNums
           + U("\n时长: ") + songLength
           + U("\n比特率: ") + songBitRate)
{
    addAndMakeVisible(No_);
    addAndMakeVisible(name);
    addAndMakeVisible(composer);
    addAndMakeVisible(album);
    addAndMakeVisible(style);
    addAndMakeVisible(mPlayAndStopButton);
    addAndMakeVisible(mWhatsMore);
}
void EachSong::resized(){

}
void EachSong::paint(juce::Graphics& g){
    
}

AllMusicComponent::AllMusicComponent(){

}

allMusicPage::allMusicPage(){
    addAndMakeVisible(refreshButton);
    addAndMakeVisible(selectFileButton);
    addAndMakeVisible(allMusicLabel);
    addAndMakeVisible(mViewPort);
    addAndMakeVisible(numSongsLabel);
    addAndMakeVisible(sortWayLabel);
    // addAndMakeVisible(sortWaysComboBox);
    addAndMakeVisible(mUpDownButton);

    // sortWaysComboBox.addItem(U("标题"));
    // sortWaysComboBox.addItem(U("作者"));
    // sortWaysComboBox.addItem(U("专辑"));
    // sortWaysComboBox.addItem(U("添加时间"));
    // sortWaysComboBox.addItem(U("音乐风格"));
    // sortWaysComboBox.addItem(U("歌曲时长"));
    // sortWaysComboBox.setSelectedId(1);

    selectFileButton.addListener(this);

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
    // sortWaysComboBox.setBounds(row2.removeFromRight(140).reduced(10));
    sortWayLabel.setBounds(row2.removeFromRight(90).reduced(10));

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