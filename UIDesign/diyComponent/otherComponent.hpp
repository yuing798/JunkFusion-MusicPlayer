#pragma once
#include "FontAbout/font.h"
#include "constants.h"
#include "juce_animation/juce_animation.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>
#include <memory>

struct YColor{
    juce::Colour black = juce::Colours::black;
    juce::Colour white {0xfff9f9f9};//"#f9f9f9"这个不是纯白色，因为纯白色太亮了
    juce::Colour shallowGrey {0xffd8d6d6};//"#d8d6d6"
    juce::Colour midGrey {0xffc5c3c3};//"#c5c3c3"
    juce::Colour darkGrey{0xffababab};//"#ababab"
    juce::Colour blackGrey{0xff706f6f};//"#706f6f"
    juce::Colour greyBlue {0xff6e75dd};//"#6e75dd"
    juce::Colour greyGreen {0xff33c4de};//"#33c4de"
    juce::Colour transparent {0x00000000};//"#000000"
    //注意juce的透明度要放在首位
};

extern YColor ycolor;

struct YLabel : public juce::Label
{

    YLabel();
    YLabel(juce::String text);
};
struct littleLabel : juce::Label{
    
    littleLabel();
    littleLabel(juce::String text);
};//字号比较小一点的标签
struct BigLabel : juce::Label{
    
    BigLabel();
    BigLabel(juce::String text);
};//大大大字号标签

class YSlider : public juce::Slider{
public:
    YSlider();
};




struct rotarySlider : public juce::Slider{

    rotarySlider();
    
};

struct verticalSlider : public juce::Slider{
    verticalSlider();
};
struct EllipsisToolTip : juce::DrawableButton{
//一个省略号图形，悬停显示注释
    EllipsisToolTip(const juce::String& text);
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    std::unique_ptr<juce::Drawable> svg;
};

