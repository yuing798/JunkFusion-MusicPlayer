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

private:

};