#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "buttons.hpp"
#include "fileMessage.hpp"
#include "juce_animation/juce_animation.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherComponent.hpp"
#include <memory>

class EachStream : public juce::Component{
private:
    PlayStopButton mPlayStopButton;
    YLabel mCountLabel;
    YLabel mNumChannels;
    YLabel mBitRate;
    YLabel mDecoderName;
    YLabel mBitDepth;
public:
    EachStream(SongInfo::stream&);
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EachStream)
};

class MultiStreamPopupWindow : public juce::Component{
private:
    static constexpr float kPopupWidth = 400.0f;
    static constexpr float kImageSize = 200.0f;
    static constexpr float kPadding = 10.0f;

    juce::Image mImage;
    juce::String mText;
    juce::String mTitle;
    juce::Font mTitleFont{juce::FontOptions().withHeight(18.0f).withStyle("Bold")};
    juce::Font mTextFont{juce::FontOptions().withHeight(16.0f)};
    juce::TextLayout mTextLayout;
    float mTextHeight{0.0f};
    juce::ComponentDragger mDragger;

public:
    svgButton xButton{
        U("关闭"),
        juce::Drawable::createFromImageData(BinaryData::x_svg, BinaryData::x_svgSize)
    };
    MultiStreamPopupWindow(juce::Image image, juce::String text, juce::String title);
    ~MultiStreamPopupWindow();
    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void resetPosition();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiStreamPopupWindow)

};

class MultiStreamCardButton : juce::TextButton{
    MultiStreamCardButton();
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class MultiStreamCard : public juce::Component,public juce::Button::Listener{
private:

    MultiStreamPopupWindow mPopupWindow;
    std::unique_ptr<juce::VBlankAnimatorUpdater> mVBlankAnimatorUpdater;
    juce::Animator mOpenAnimator;
    juce::Animator mCloseAnimator;
    bool mIsPopupVisible{false};

    yTextButton cardButton{U("多流音频")};
public:

    MultiStreamCard(juce::Image image, juce::String text, juce::String title);
    ~MultiStreamCard();
    void resized() override;
    void paint(juce::Graphics&) override;
    void buttonClicked(juce::Button*) override;
    void setPopupNoSee();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiStreamCard)
};
