#include "allMusicPage.hpp"
#include "FontAbout/font.h"
#include "juce_graphics/juce_graphics.h"
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
    addAndMakeVisible(upAndDownButton);

    sortWaysComboBox.addItem(U("标题"), 1);
    sortWaysComboBox.addItem(U("作者"), 2);
    sortWaysComboBox.addItem(U("专辑"), 3);
    sortWaysComboBox.addItem(U("添加时间"), 4);
    sortWaysComboBox.addItem(U("音乐风格"), 5);
    sortWaysComboBox.addItem(U("歌曲时长"), 6);



}
void allMusicPage::resized(){

    auto local = getLocalBounds();
}
void allMusicPage::paint(juce::Graphics& g){

}