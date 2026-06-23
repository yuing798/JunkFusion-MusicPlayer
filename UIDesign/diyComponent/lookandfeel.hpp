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

    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override;//重写提示悬浮窗外观

    // void drawComboBox	(	Graphics &	,
    //     int	width,
    //     int	height,
    //     bool	isButtonDown,
    //     int	buttonX,
    //     int	buttonY,
    //     int	buttonW,
    //     int	buttonH,
    //     ComboBox &	 ) override;//重绘comboBox

    // void drawPopupMenuItemWithOptions	(	Graphics &	,
    //     const Rectangle< int > &	area,
    //     bool	isHighlighted,
    //     const PopupMenu::Item &	item,
    //     const PopupMenu::Options &	 ) override;

    // void drawPopupMenuBackgroundWithOptions	(	Graphics &	,
    //     int	width,
    //     int	height,
    //     const PopupMenu::Options &	 ) override;


private:
    // juce::Typeface::Ptr customTypeface; // 用于存储全局字体的指针

};