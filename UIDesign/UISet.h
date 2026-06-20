#pragma once
#include "constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>


struct YColor{
    juce::Colour black = juce::Colours::black;
    juce::Colour white {0xfff0eded};//"#f0eded"这个不是纯白色，因为纯白色太亮了
    juce::Colour shallowGrey {0xffd8d6d6};//"#d8d6d6"
    juce::Colour midGrey {0xffc5c3c3};//"#c5c3c3"
    juce::Colour darkGrey{0xff575656};//"#999898"
    juce::Colour blackGrey{0xff706f6f};//"#706f6f"
    juce::Colour greyBlue {0xff6e75dd};//"#6e75dd"
    juce::Colour greyGreen {0xff33c4de};//"#33c4de"
    juce::Colour transparent {0x00000000};//"#000000"
    //注意juce的透明度要放在首位
};

extern YColor ycolor;

struct YLookAndFeel : public juce::LookAndFeel_V4{

    YLookAndFeel();

    juce::Font getComboBoxFont (juce::ComboBox& box) override;//设置comboBox字体

    juce::Font getPopupMenuFont() override;//设置下拉框字体

    //重写圆形旋钮外观
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                        juce::Slider& slider) override;

    //重写竖状条形旋钮外观
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override;//重写字体

private:
    // juce::Typeface::Ptr customTypeface; // 用于存储全局字体的指针

};

struct YLabel : public juce::Label
{
    YLabel(juce::String text);
    
};

class YSlider : public juce::Slider{

public:
    YSlider();
};

//经典的音频开启关闭按钮
class PlayStopButton : public juce::ToggleButton
{
public:

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct rotarySlider : public juce::Slider{

    rotarySlider();
    
};

struct verticalSlider : public juce::Slider{
    verticalSlider();
};

struct yTextButton : public juce::TextButton{//这个按钮可以点击，但是不会发生状态翻转
    yTextButton(juce::String initText,bool shouldBeToggle = true);
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

struct yComboBox : public juce::ComboBox{
    yComboBox();
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

// struct transparentButton : public juce::ToggleButton{
//     transparentButton(juce::String text);
//     void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
// };

// struct unchangeButton : transparentButton{
// //这个按钮点击了不会发生文字翻转
// };