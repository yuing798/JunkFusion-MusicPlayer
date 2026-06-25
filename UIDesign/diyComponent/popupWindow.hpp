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

//点击按钮实现一个跳到屏幕中间的窗口



class popupWindow : public juce::Component{
private:
    juce::String mTitle;
    juce::Font mTitleFont{juce::FontOptions().withHeight(18.0f).withStyle("Bold")};
    std::unique_ptr<juce::Component> mWindowComponent;
    juce::ComponentDragger mDragger;
public:
    svgButton xButton{
        U("关闭"),
        juce::Drawable::createFromImageData(BinaryData::x_svg, BinaryData::x_svgSize)
    };
    popupWindow(juce::String title,std::unique_ptr<juce::Component>);
    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void resetPosition();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(popupWindow)

};//点击省略号按钮会在应用的中间区域画出一个矩形来显示歌曲的额外信息

class PopupWindowButton : public juce::Component,public juce::Button::Listener{
private:
    std::unique_ptr<juce::Button> mButton;
    std::unique_ptr<popupWindow> mPopupWindow;
    std::unique_ptr<juce::VBlankAnimatorUpdater> mVBlankAnimatorUpdater;
    juce::Animator mOpenAnimator;
    juce::Animator mCloseAnimator;
    bool mIsPopupVisible{false};

    
public:

    PopupWindowButton(std::unique_ptr<juce::Button> button,juce::String windowTitle,std::unique_ptr<juce::Component> windowComponent);
    ~PopupWindowButton();
    void resized() override;
    void paint(juce::Graphics&) override;
    void buttonClicked(juce::Button*) override;
    void setPopupNoSee();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupWindowButton)
};


