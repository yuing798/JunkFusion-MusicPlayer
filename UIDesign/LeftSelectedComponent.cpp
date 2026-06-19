#include "LeftSelectedComponent.hpp"
#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

selectedButton::selectedButton(juce::String initText){
    setButtonText(initText);
    setClickingTogglesState(true);//允许点击切换状态
}

void selectedButton::paintButton (juce::Graphics& g, 
    bool shouldDrawButtonAsHighlighted, 
    bool shouldDrawButtonAsDown){

    auto local = getLocalBounds().toFloat();

    if(getToggleState()){
        g.setColour(ycolor.greyBlue);
        g.fillRoundedRectangle(local,6.0f);
    }else{
        g.setColour(ycolor.transparent);
        g.fillRoundedRectangle(local,6.0f);
        if(shouldDrawButtonAsHighlighted){
            g.setColour(ycolor.midGrey);
            g.fillRoundedRectangle(local,6.0f);
        }
    }
    g.setColour (ycolor.black);
    g.setFont (juce::FontOptions { 18.0f });
    // if (shouldDrawButtonAsHighlighted) {
    //     local = local.translated(0.0f, 1.0f);
    // }

    g.drawText (getButtonText(), local, juce::Justification::centred, true);
}
LeftSelectedComponent::LeftSelectedComponent()
{
    // 将所有按钮指针加入 vector
    buttons = {
        // &musicLibraryBrowse,
        &allMusic,
        &myLike,
        &recentPlay,
        // &categoryBrowse,
        &artist,
        &album,
        &playlist,
        &genre,
        // &featureSection,
        &aiAssistant,
        &effects,
        &equalizer,
        &speakerArray,
        &settings
    };

    // 将按钮添加到内容组件并设为可见
    for (auto* btn : buttons)
    {
        mContentComponent.addAndMakeVisible(btn);
    }

    // 设置 Viewport：垂直可滚动，水平不滚动
    mViewPort.setViewedComponent(&mContentComponent, false);
    mViewPort.setScrollBarsShown(true, false);
    addAndMakeVisible(mViewPort);

    // 设置内容组件初始尺寸（按钮纵向排列）
    const int buttonHeight = 32;
    const int contentWidth  = 200;
    mContentComponent.setSize(contentWidth,
                              static_cast<int>(buttons.size()) * buttonHeight);
}

void LeftSelectedComponent::resized()
{
    auto area = getLocalBounds();
    mViewPort.setBounds(area);

    const int buttonHeight = 32;
    auto contentArea = mContentComponent.getLocalBounds();

    for (auto* btn : buttons)
    {
        btn->setBounds(contentArea.removeFromTop(buttonHeight));
    }
}

void LeftSelectedComponent::paint(juce::Graphics& g){
    g.setColour(ycolor.shallowGrey);
    g.fillAll();
}