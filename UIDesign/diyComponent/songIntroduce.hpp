#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "buttons.hpp"
#include "juce_animation/juce_animation.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherComponent.hpp"
#include <memory>

class popupWindow : public juce::Component{
private:
    
public:
    svgButton xButton{
        U("关闭"),
        juce::Drawable::createFromImageData(BinaryData::x_svg, BinaryData::x_svgSize)
    };
    popupWindow();
    ~popupWindow();
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(popupWindow)

};//点击省略号按钮会在应用的中间区域画出一个矩形来显示歌曲的额外信息

class SongIntroduce : public juce::Component,public juce::Button::Listener{
private:
    
    std::unique_ptr<juce::VBlankAnimatorUpdater> mVBlankAnimatorUpdater;
    juce::Animator mOpenAnimator;
    juce::Animator mCloseAnimator;//juce::Animator里面已经包含了智能指针管理了
    juce::Rectangle<int> popupRect;
    
    popupWindow mPopupWindow;

    float popupWidth{0.0f};
    float popupHeight{0.0f};

    float minPopupHeight{0};//popup窗口的最小数值
    juce::Component* topComponent;
    juce::Font fontSize{juce::FontOptions().withHeight (16.0f)};

    svgButton whatsmoreButton{
        U("更多信息"),
        juce::Drawable::createFromImageData(BinaryData::whatsMore_svg,BinaryData::whatsMore_svgSize)
    };
public:
    
    SongIntroduce(juce::String& text);
    ~SongIntroduce();
    void resized() override;
    void paint(juce::Graphics&) override;
    void buttonClicked(juce::Button*) override;
    void setPopupNoSee();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongIntroduce)
};