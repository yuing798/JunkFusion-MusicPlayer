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
    // 将所有按钮指针加入 vector
    buttons.add(&allMusic);
    buttons.add(&myLike);
    buttons.add(&recentPlay);
    buttons.add(&artist);
    buttons.add(&album);
    buttons.add(&playlist);
    buttons.add(&genre);
    buttons.add(&aiAssistant);
    buttons.add(&effects);
    buttons.add(&equalizer);
    buttons.add(&speakerArray);
    buttons.add(&settings);
    // buttons.add(&);

    for(auto& button : buttons){
        addAndMakeVisible(button);
    }

    labelArray.add(&musicLibraryBrowse);
    labelArray.add(&categoryBrowse);
    labelArray.add(&featureSection);

    for(auto& label : labelArray){
        addAndMakeVisible(label);
    }
    juce::Path path1;
    juce::Path path2;
    juce::Path path3;
    paths.add(&path1);
    paths.add(&path2);
    paths.add(&path3);
}

void LeftSelectedComponent::resized()
{
    auto local = getLocalBounds();
    // auto width = local.getWidth();
    labelArray[0]->setBounds(local.removeFromTop(24));
    paths[0]->startNewSubPath(0,local.getY());
    local.removeFromTop(5);
    for(int i = 0; i < 3; i++){
        buttons[i]->setBounds(local.removeFromTop(32).reduced(3));
    }
    labelArray[1]->setBounds(local.removeFromTop(24));
    paths[1]->startNewSubPath(0,local.getY());
    local.removeFromTop(5);
    for(int i = 3; i < 7; i++){
        buttons[i]->setBounds(local.removeFromTop(32).reduced(3));
    }
    labelArray[2]->setBounds(local.removeFromTop(24));
    paths[2]->startNewSubPath(0,local.getY());
    local.removeFromTop(5);
    for(int i = 7; i < buttons.size(); i++){
        buttons[i]->setBounds(local.removeFromTop(32).reduced(3));
    }

}

void LeftSelectedComponent::paint(juce::Graphics& g){
    g.setColour(ycolor.transparent);
    g.fillAll();

    g.setColour(ycolor.midGrey);
    g.strokePath(*paths[0], juce::PathStrokeType (3.0f));
    g.strokePath(*paths[1], juce::PathStrokeType (3.0f));
    g.strokePath(*paths[2], juce::PathStrokeType (3.0f));

}