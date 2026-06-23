#include "LeftColumn.hpp"
#include "BinaryData.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>

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
    for(auto& button : buttons){
        button.addListener(this);//注册监听，实现单选逻辑
        addAndMakeVisible(button);
    }

    for(auto& label : labelArray){
        addAndMakeVisible(label);
    }

    height = (10+50+10)*3+40*buttons.size()+60;

    logo.setImage(juce::ImageCache::getFromMemory(BinaryData::junkfusion_png, BinaryData::junkfusion_pngSize));
    logo.setImagePlacement(juce::RectanglePlacement::centred);
    addAndMakeVisible(logo);


}

LeftSelectedComponent::~LeftSelectedComponent(){

}

void LeftSelectedComponent::resized()
{
    auto local = getLocalBounds();
    auto width = local.getWidth();

    logo.setBounds(local.removeFromTop(60).reduced(5));

    local.removeFromTop(10);
    paths[0].startNewSubPath(0,local.getY());
    paths[0].lineTo(width,local.getY());
    paths[0].closeSubPath();
    labelArray[0].setBounds(local.removeFromTop(50));
    paths[1].startNewSubPath(0,local.getY());
    paths[1].lineTo(width,local.getY());
    paths[1].closeSubPath();

    local.removeFromTop(10);
    for(size_t i = 0; i < 3; i++){
        buttons[i].setBounds(local.removeFromTop(40).reduced(3));
    }

    local.removeFromTop(10);
    paths[2].startNewSubPath(0,local.getY());
    paths[2].lineTo(width,local.getY());
    paths[2].closeSubPath();
    labelArray[1].setBounds(local.removeFromTop(50));
    paths[3].startNewSubPath(0,local.getY());
    paths[3].lineTo(width,local.getY());
    paths[3].closeSubPath();
    local.removeFromTop(10);
    for(size_t i = 3; i < 7; i++){
        buttons[i].setBounds(local.removeFromTop(40).reduced(3));
    }

    local.removeFromTop(10);
    paths[4].startNewSubPath(0,local.getY());
    paths[4].lineTo(width,local.getY());
    paths[4].closeSubPath();
    labelArray[2].setBounds(local.removeFromTop(50));
    paths[5].startNewSubPath(0,local.getY());
    paths[5].lineTo(width,local.getY());
    paths[5].closeSubPath();
    local.removeFromTop(10);
    for(size_t i = 7; i < buttons.size(); i++){
        buttons[i].setBounds(local.removeFromTop(40).reduced(3));
    }
    
}

void LeftSelectedComponent::paint(juce::Graphics& g){
    g.setColour(ycolor.transparent);
    g.fillAll();

    g.setColour(ycolor.midGrey);
    for(auto& path : paths){
        g.strokePath(path, PathStrokeType(5.0f));
    }

}

void LeftSelectedComponent::buttonClicked(juce::Button* clickedButton)
{
    // 如果当前点击的按钮被关闭了（toggleState == false），不允许取消选中
    // 保证始终有一个按钮处于选中状态（收音机行为）
    if (!clickedButton->getToggleState())
    {
        clickedButton->setToggleState(true, juce::dontSendNotification);
        return;
    }

    // 关闭其他所有按钮，只保留当前点击的按钮
    for (size_t i = 0; i < buttons.size(); i++)
    {
        if (&buttons[i] != clickedButton)
        {
            buttons[i].setToggleState(false, juce::dontSendNotification);
        }
        if (buttons[i].getToggleState())
        {
            selectID = static_cast<int>(i);
            if (onSelectionChanged)
                onSelectionChanged(selectID);
        }
    }
}


LeftColumn::LeftColumn(){
    mViewPort.setViewedComponent(&mLeftSelectedComponent,false);

    // 参数：显示垂直滚动条，隐藏水平滚动条
    mViewPort.setScrollBarsShown(true, false, true, false);
    addAndMakeVisible(mViewPort);
}

void LeftColumn::resized(){
    mViewPort.setBounds(getLocalBounds());
    mLeftSelectedComponent.setSize(getWidth(), mLeftSelectedComponent.height);
}

void LeftColumn::paint(juce::Graphics& g){
    g.setColour(ycolor.shallowGrey);
    g.fillAll();
}