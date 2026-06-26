
#include "FontAbout/font.h"
#include "juce_events/juce_events.h"
#include "otherComponent.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>
#include "./multiStreamCard.hpp"

EachStream::EachStream(SongInfo::stream& stream){
    addAndMakeVisible(mPlayStopButton);
    addAndMakeVisible(mCountLabel);
    addAndMakeVisible(mNumChannels);
    addAndMakeVisible(mBitDepth);
    addAndMakeVisible(mBitRate);
    addAndMakeVisible(mDecoderName);
    addAndMakeVisible(*mEllipsisToolTip);

    mCountLabel.setText(juce::String(stream.streamCount), dontSendNotification);
    mNumChannels.setText(juce::String(stream.numChannels), dontSendNotification);
    mBitDepth.setText(juce::String(stream.bitDepth), dontSendNotification);
    mBitRate.setText(juce::String(stream.bitRate), dontSendNotification);
    mDecoderName.setText(juce::String(stream.codecName), dontSendNotification);

    juce::String isMusic = stream.isMusic ? U("是") : U("否");
    juce::String isAIprocessed = stream.aiProcessed ? U("是") : U("否");

    mEllipsisToolTip = std::make_unique<EllipsisToolTip>(longUTF8(
        "AI分析体裁:",stream.aiGenre,
        "\nAI分析情绪:",stream.aiMood,
        "\n是否为音乐资源",isMusic,
        "\n是否已经进行过AI分析:",isAIprocessed,
        "\nBPM:",juce::String(stream.bpm),
        
        "\n调性:",juce::String(stream.key),
        "\n采样率:",juce::String(stream.sampleRate),
        "\n额外信息:",stream.extraMetadata
    ));

    setSize(700, 100);
}
void EachStream::resized(){
    auto local{getLocalBounds().reduced(5)};
    mPlayStopButton.setBounds(local.removeFromLeft(getHeight()).reduced(30));
    mCountLabel.setBounds(local.removeFromLeft(50).reduced(8));
    mNumChannels.setBounds(local.removeFromLeft(50).reduced(8));
    mBitDepth.setBounds(local.removeFromLeft(100).reduced(10));
    mBitRate.setBounds(local.removeFromLeft(100).reduced(10));
    mDecoderName.setBounds(local.removeFromLeft(200).reduced(10));
    mEllipsisToolTip->setBounds(local.removeFromLeft(getHeight()).reduced(30));
}
void EachStream::paint(juce::Graphics& g){
    auto local{getLocalBounds().toFloat()};
    g.setColour(ycolor.shallowGrey);
    g.fillRoundedRectangle(local,15.0f);
}

MultiStreamCardButton::MultiStreamCardButton(){
    setButtonText(U("多流音频"));
    setClickingTogglesState(true);
}
void MultiStreamCardButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){
    auto local{getLocalBounds().toFloat()};
    g.setColour(ycolor.greyBlue);
    g.fillRoundedRectangle(local,3.0f);
    g.setColour(ycolor.white);
    g.setFont(juce::FontOptions{}.withHeight(16.0f));
    g.drawText(getButtonText(),local,juce::Justification::centred,true);
}

StreamCardComponent::StreamCardComponent(){
    auto height{streams.size()*(streams[0].getHeight())+10.0f};
    auto width{streams[0].getWidth()};
    for(auto& stream:streams){
        addAndMakeVisible(stream);
    }
    setSize(width,height);

}
void StreamCardComponent::resized(){
    float currentY{0.0f};
    for(auto& stream:streams){
        stream.setTopLeftPosition(0,currentY);
        currentY += stream.getHeight();
    }
}