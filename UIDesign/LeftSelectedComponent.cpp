#include "LeftSelectedComponent.hpp"
#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <cstddef>
#include <type_traits>

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
        addAndMakeVisible(button);
    }

    for(auto& label : labelArray){
        addAndMakeVisible(label);
    }

    height = 5 * 3 + 24 * labelArray.size() + 32 * buttons.size();
}

LeftSelectedComponent::~LeftSelectedComponent(){

}

void LeftSelectedComponent::resized()
{
    auto local = getLocalBounds();
    // auto width = local.getWidth();
    labelArray[0].setBounds(local.removeFromTop(24));
    paths[0].startNewSubPath(0,local.getY());
    local.removeFromTop(5);
    for(int i = 0; i < 3; i++){
        buttons[i].setBounds(local.removeFromTop(32).reduced(3));
    }
    labelArray[1].setBounds(local.removeFromTop(24));
    paths[1].startNewSubPath(0,local.getY());
    local.removeFromTop(5);
    for(int i = 3; i < 7; i++){
        buttons[i].setBounds(local.removeFromTop(32).reduced(3));
    }
    labelArray[2].setBounds(local.removeFromTop(24));
    paths[2].startNewSubPath(0,local.getY());
    local.removeFromTop(5);
    for(int i = 7; i < buttons.size(); i++){
        buttons[i].setBounds(local.removeFromTop(32).reduced(3));
    }
    
}

void LeftSelectedComponent::paint(juce::Graphics& g){
    g.setColour(ycolor.transparent);
    g.fillAll();

    g.setColour(ycolor.midGrey);
    g.strokePath(paths[0], juce::PathStrokeType (3.0f));
    g.strokePath(paths[1], juce::PathStrokeType (3.0f));
    g.strokePath(paths[2], juce::PathStrokeType (3.0f));

}

LeftComponent::LeftComponent(){
    mViewPort.setViewedComponent(&mLeftSelectedComponent,false);

    // 参数：显示垂直滚动条，隐藏水平滚动条
    mViewPort.setScrollBarsShown(true, false, true, false);
    addAndMakeVisible(mViewPort);
}

void LeftComponent::resized(){
    mViewPort.setBounds(getLocalBounds());
    mLeftSelectedComponent.setSize(getWidth(), mLeftSelectedComponent.height);
}

void LeftComponent::paint(juce::Graphics& g){
    g.setColour(ycolor.shallowGrey);
    g.fillAll();
}