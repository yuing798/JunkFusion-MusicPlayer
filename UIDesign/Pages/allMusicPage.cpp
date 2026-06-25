#include "allMusicPage.hpp"
#include "FontAbout/font.h"
#include "databaseManage.hpp"
#include "fileMessage.hpp"
#include "fileUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"

EachSong::EachSong(int songNo,SongInfo info)

{
    addAndMakeVisible(mPlayAndStopButton);

    No_.setText(juce::String(songNo), juce::dontSendNotification);
    addAndMakeVisible(No_);
    
    juce::String songTitle{""};//歌曲名称
    if(!info.title.empty()){
        songTitle = juce::String(info.title);
    }else{
        songTitle = juce::File(info.filePath).getFileNameWithoutExtension();
    }//如果文件没有title，直接使用文件的stem名称
    mName.setText(songTitle, juce::dontSendNotification);
    addAndMakeVisible(mName);

    juce::String artistName{U("未知")};
    if(!info.artist.empty()){
        artistName = juce::String(info.artist);
    }
    mArtist.setText(artistName, dontSendNotification);
    addAndMakeVisible(mArtist);

    juce::String albumName{U("未知")};
    if(!info.album.empty()){
        albumName = juce::String(info.album);
    }
    mAlbum.setText(albumName, dontSendNotification);
    addAndMakeVisible(mAlbum);
    
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
    addAndMakeVisible(sortWaysComboBox);
    addAndMakeVisible(mUpDownButton);

    sortWaysComboBox.addItem(U("标题"));
    sortWaysComboBox.addItem(U("作者"));
    sortWaysComboBox.addItem(U("专辑"));
    sortWaysComboBox.addItem(U("添加时间"));
    sortWaysComboBox.addItem(U("音乐风格"));
    sortWaysComboBox.addItem(U("歌曲时长"));
    // sortWaysComboBox.setSelectedId(1);

    selectFileButton.addListener(this);

    // addAndMakeVisible(mSongIntroduce);

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
    sortWaysComboBox.setBounds(row2.removeFromRight(140).reduced(10));
    sortWayLabel.setBounds(row2.removeFromRight(90).reduced(10));

    allMusicLabel.setBounds(row1.removeFromLeft(90));
    numSongsLabel.setBounds(row1.removeFromLeft(90));
    // mSongIntroduce.setBounds(row1.removeFromLeft(row2.getHeight()));

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