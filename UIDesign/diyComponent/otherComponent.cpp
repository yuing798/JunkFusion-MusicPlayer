#include "BinaryData.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>
#include <utility>
#include "./otherComponent.hpp"

YColor ycolor;

YLabel::YLabel(){
    setFont (juce::FontOptions (18.0f));

    setColour (juce::Label::textColourId, juce::Colours::black);
    setJustificationType (juce::Justification::centred);
}
YLabel::YLabel(juce::String text) 
:YLabel(){
    setText(text, juce::dontSendNotification);
    
}
littleLabel::littleLabel(){
    setFont (juce::FontOptions (15.0f));
    setColour (juce::Label::textColourId, juce::Colours::darkgrey);
    setJustificationType (juce::Justification::centred);
}
littleLabel::littleLabel(juce::String text)
:littleLabel(){

    setText(text, juce::dontSendNotification);
}//字号小一点的标签
BigLabel::BigLabel(){
    setFont (juce::FontOptions (21.0f).withStyle("Bold"));
    setColour (juce::Label::textColourId, juce::Colours::black);
    setJustificationType (juce::Justification::centred);
}
BigLabel::BigLabel(juce::String text)
:BigLabel(){

    setText(text, juce::dontSendNotification);

}//大标签!!!


YSlider::YSlider(){
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::black);
}

rotarySlider::rotarySlider(){

    setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 30);
}

verticalSlider::verticalSlider(){
    setSliderStyle(juce::Slider::LinearVertical);
    setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 30);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentWhite);
    setColour(juce::Slider::textBoxHighlightColourId, juce::Colours::transparentWhite);
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::black);
}
EllipsisToolTip::EllipsisToolTip(const juce::String& text)
:juce::DrawableButton ("", juce::DrawableButton::ImageFitted)
{

    setTooltip(text);
    setClickingTogglesState(false);
    svg = juce::Drawable::createFromImageData(BinaryData::ellipsis_svg, BinaryData::ellipsis_svgSize);
}

void EllipsisToolTip::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){

    g.setColour(ycolor.shallowGrey);
    if(shouldDrawButtonAsHighlighted) g.setColour(ycolor.midGrey);
    auto bounds = getLocalBounds().toFloat();
    auto width{bounds.getWidth()};
    auto reducedWidth{width * (1.0f - 0.707f)};//因为图形是圆形的，所以需要把边缘削去一部分
    g.fillRoundedRectangle(bounds, width / 2.0f);
    // 绘制 SVG，居中适应按钮区域
    svg.get()->drawWithin(g, bounds.reduced(reducedWidth).reduced(1.0f),
                            juce::RectanglePlacement::centred,
                              1.0f);
}

