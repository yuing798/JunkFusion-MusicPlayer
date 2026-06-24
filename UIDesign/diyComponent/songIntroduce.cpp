
#include "songIntroduce.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

popupWindow::popupWindow(){
    xButton.setClickingTogglesState(true);
}
popupWindow::~popupWindow(){

}
void popupWindow::resized(){

}
void popupWindow::paint(juce::Graphics&g){
    auto local{getLocalBounds().toFloat()};
    g.setColour(ycolor.white);
    g.fillRoundedRectangle(local,4.0f);
    g.setColour(ycolor.darkGrey);
    g.drawRoundedRectangle(0,0,local.getWidth(),local.getHeight(),4.0f,3.0f);
}

SongIntroduce::SongIntroduce(juce::String text)
:mVBlankAnimatorUpdater(std::make_unique<juce::VBlankAnimatorUpdater> (this)),


// --- 打开动画器 (easeOut, 200ms) ---
mOpenAnimator(juce::ValueAnimatorBuilder{}
.withDurationMs (200)
.withEasing (juce::Easings::createEaseOut())//淡出
.withOnStartCallback ([this]
{

})
.withValueChangedCallback ([this] (float progress)
{

})
.build()),

// --- 关闭动画器 (easeIn, 200ms) ---
mCloseAnimator(juce::ValueAnimatorBuilder{}
.withDurationMs (200)
.withEasing (juce::Easings::createEaseIn())//淡入
.withValueChangedCallback ([this] (float progress)
{

})
.withOnCompleteCallback ([this]
{

})
.build())
{
    mVBlankAnimatorUpdater->addAnimator (mOpenAnimator);
    mVBlankAnimatorUpdater->addAnimator (mCloseAnimator);
    
    juce::Component* topComponent{nullptr};
    topComponent = getTopLevelComponent();
    topComponent->addAndMakeVisible(mPopupWindow);

    whatsmoreButton.setClickingTogglesState(true);
    
    popupWidth = topComponent->getLocalBounds().getWidth() / 5.0f;
    minPopupHeight = topComponent->getLocalBounds().getHeight() / 3.0f;//最小高为3分之一,宽度固定

    juce::AttributedString attributedText{text};
    attributedText.setFont(fontSize);
    attributedText.setWordWrap(juce::AttributedString::WordWrap::byWord);//单词换行

    juce::TextLayout layout;
    layout.createLayout(attributedText,popupWidth);
    popupHeight = layout.getHeight();
    popupHeight = juce::jmax(popupHeight,minPopupHeight);

    //注册监听
    whatsmoreButton.addListener(this);
    mPopupWindow.xButton.addListener(this);

    addAndMakeVisible(whatsmoreButton);

}
SongIntroduce::~SongIntroduce(){

}
void SongIntroduce::resized(){
    auto screen{getScreenBounds().toFloat()};
    mPopupWindow.setBounds(
        (screen.getWidth()-popupWidth)/2.0f,
        (screen.getHeight()-popupHeight)/2.0f,
        popupWidth,
        popupHeight
    );
    whatsmoreButton.setBounds(getLocalBounds());
}
void SongIntroduce::paint(juce::Graphics&){

}
void SongIntroduce::buttonClicked(juce::Button* button){

    if(button == &whatsmoreButton){
        if(button->getToggleState()){
            mPopupWindow.setVisible(true);
        }else{
            setPopupNoSee();
        }
    }
    if(button == &mPopupWindow.xButton){
        setPopupNoSee();
    }
}
void SongIntroduce::setPopupNoSee(){
    mPopupWindow.setVisible(false);
}