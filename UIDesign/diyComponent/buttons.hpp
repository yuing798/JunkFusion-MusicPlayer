#pragma once
#include "FontAbout/font.h"
#include "constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>
#include <memory>


//经典的音频开启关闭按钮
class PlayStopButton : public juce::ToggleButton
{
public:

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};
struct yTextButton : public juce::TextButton{//这个按钮可以点击，但是不会发生状态翻转
    yTextButton();
    yTextButton(juce::String initText);
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};



struct plusButton : public juce::ToggleButton{//设置加号形状
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};
struct minusButton : public juce::ToggleButton{//设置减号形状
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct plusAndMinusButton{
    plusButton yPlusButton;
    minusButton yMinusButton;
};//因为加减号按钮肯定是成对出现的

struct noneButton : public juce::ToggleButton{//这个按钮只允许点击，没其他用处了，在UI界面什么都不显示
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct svgButton : public juce::DrawableButton{//每个可以点击的svg矢量的按钮，不可进行状态翻转
    svgButton(const juce::String& buttonName,std::unique_ptr<juce::Drawable>);
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    std::unique_ptr<juce::Drawable> svg;
};

struct doubleSvgButton : public juce::DrawableButton{//点击后能够根据toggle状态改变svg图片的按钮
    doubleSvgButton(const juce::String& name1,std::unique_ptr<juce::Drawable> svg1,
        const juce::String& name2,std::unique_ptr<juce::Drawable> svg2);

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::String name1,name2;
    std::unique_ptr<juce::Drawable> svg1,svg2;
};

struct playPauseButton : doubleSvgButton{
    playPauseButton();
};
struct upDownButton : doubleSvgButton{
    upDownButton();
};