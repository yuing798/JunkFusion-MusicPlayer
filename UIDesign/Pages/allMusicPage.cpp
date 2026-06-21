#include "allMusicPage.hpp"
#include "FontAbout/font.h"
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
    addAndMakeVisible(sortWaysComboBox);
    addAndMakeVisible(mUpDownButton);

    sortWaysComboBox.addItem(U("标题"), 1);
    sortWaysComboBox.addItem(U("作者"), 2);
    sortWaysComboBox.addItem(U("专辑"), 3);
    sortWaysComboBox.addItem(U("添加时间"), 4);
    sortWaysComboBox.addItem(U("音乐风格"), 5);
    sortWaysComboBox.addItem(U("歌曲时长"), 6);
    sortWaysComboBox.setSelectedId(1);

}
void allMusicPage::resized(){

    auto local = getLocalBounds();
    auto height = local.getHeight();
    mViewPort.setBounds(local.removeFromBottom(height * 0.85f));
    auto row2 = local.removeFromBottom(height * 0.1f).reduced(10.0f);
    auto row1 = local.reduced(10.0f);

    selectFileButton.setBounds(row2.removeFromLeft(80));
    refreshButton.setBounds(row2.removeFromLeft(row2.getHeight()));
   mUpDownButton.setBounds(row2.removeFromRight(row2.getHeight()));
    sortWaysComboBox.setBounds(row2.removeFromRight(80));
    sortWayLabel.setBounds(row2.removeFromRight(70));

    allMusicLabel.setBounds(row1.removeFromLeft(90));
    numSongsLabel.setBounds(row1.removeFromLeft(90));

}
// void allMusicPage::paint(juce::Graphics& g){

// }