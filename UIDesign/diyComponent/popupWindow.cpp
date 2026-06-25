#include "popupWindow.hpp"
#include "buttons.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

// ======================================================================
// popupWindow 实现
// ======================================================================

popupWindow::popupWindow(juce::Image image, juce::String text, juce::String title)
    : mImage(image), mText(text), mTitle(title)
{
    xButton.setClickingTogglesState(true);
    addAndMakeVisible(xButton);

    // 计算文本区域高度
    juce::AttributedString attributedText{mText};
    attributedText.setFont(mTextFont);
    attributedText.setWordWrap(juce::AttributedString::WordWrap::byWord);

    float textMaxWidth = kPopupWidth - 2.0f * kPadding;
    mTextLayout.createLayout(attributedText, textMaxWidth);
    mTextHeight = mTextLayout.getHeight();

    float imageSize{kImageSize};
    if(image.isNull()){
        imageSize = 0;
    }//如果没有放置图片则删掉图片的占位区域
    float realHeight{0.0f};//如果信息太少会有一个最小高度
    realHeight = juce::jmax(350.0f,kPadding + imageSize + kPadding + mTextHeight + kPadding);

    // 总高度 = 标题行 + 间距 + 图片 + 间距 + 文本 + 间距
    float titleRowHeight = mTitleFont.getHeight() + kPadding;
    float totalHeight = titleRowHeight + realHeight;
    setSize(static_cast<int>(kPopupWidth), static_cast<int>(totalHeight));
}

popupWindow::~popupWindow() {}

void popupWindow::resized()
{
    auto bounds = getLocalBounds();
    float titleRowHeight = mTitleFont.getHeight() + kPadding;

    // xButton — 右上角，正方形
    int buttonSize = static_cast<int>(titleRowHeight);
    auto titleRow = bounds.removeFromTop(buttonSize);
    xButton.setBounds(titleRow.removeFromRight(buttonSize).reduced(4));
}

void popupWindow::paint(juce::Graphics& g)
{
    auto local{getLocalBounds().toFloat()};

    // 背景与边框
    g.setColour(ycolor.shallowGrey);
    g.fillRoundedRectangle(local, 7.0f);

    float titleRowHeight = mTitleFont.getHeight() + kPadding;
    float currentY = 0.0f;

    // 标题 — 标题行内居中
    {
        juce::Rectangle<float> titleBounds(kPadding, currentY,
                                            kPopupWidth - 2.0f * kPadding, titleRowHeight);
        g.setFont(mTitleFont);
        g.setColour(ycolor.black);
        g.drawText(mTitle, titleBounds, juce::Justification::centred, false);
        currentY += titleRowHeight + kPadding;
    }

    // 图片 — 若有图则 200×200 居中，无图则不占空间
    if (mImage.isValid())
    {
        float imageX = (kPopupWidth - kImageSize) / 2.0f;
        juce::Rectangle<float> imageBounds(imageX, currentY, kImageSize, kImageSize);
        g.drawImage(mImage, imageBounds, juce::RectanglePlacement::centred);
        currentY += kImageSize + kPadding;
    }

    // 文本 — 左对齐
    {
        juce::Rectangle<float> textBounds(kPadding, currentY,
                                           kPopupWidth - 2.0f * kPadding, mTextHeight);
        mTextLayout.draw(g, textBounds);
    }
}

void popupWindow::mouseDown(const juce::MouseEvent& e)
{
    mDragger.startDraggingComponent(this, e);
}

void popupWindow::mouseDrag(const juce::MouseEvent& e)
{
    mDragger.dragComponent(this, e, nullptr);
}

void popupWindow::resetPosition()
{
    if (auto* parent = getParentComponent())
        setCentrePosition(parent->getLocalBounds().getCentre());
}

// ======================================================================
// PopupWindowButton 实现
// ======================================================================
//原来的传参：juce::Image image, juce::String text, juce::String title

PopupWindowButton::PopupWindowButton(
    std::unique_ptr<svgButton> button,
    juce::String windowTitle,
    std::unique_ptr<juce::Component> windowComponent
)
    : 
      mVBlankAnimatorUpdater(std::make_unique<juce::VBlankAnimatorUpdater>(this)),

      // --- 打开动画器 (easeOut, 200ms) ---
      mOpenAnimator(juce::ValueAnimatorBuilder{}
                        .withDurationMs(200)
                        .withEasing(juce::Easings::createEaseOut())
                        .withOnStartCallback([this]
                        {
                            // 延迟到动画启动时才添加到顶层组件，确保组件树已建立
                            auto* topLevel = getTopLevelComponent();
                            if (mPopupWindow->getParentComponent() != topLevel)
                            {
                                if (auto* oldParent = mPopupWindow->getParentComponent())
                                    oldParent->removeChildComponent(mPopupWindow.get());
                                topLevel->addChildComponent(mPopupWindow.get());
                            }
                            mIsPopupVisible = true;
                            mPopupWindow->resetPosition();
                            mPopupWindow->setAlpha(0.0f);
                            mPopupWindow->setVisible(true);
                            mPopupWindow->xButton.setToggleState(false, juce::dontSendNotification);
                        })
                        .withValueChangedCallback([this](float progress)
                        {
                            mPopupWindow->setAlpha(progress);
                            auto cx = static_cast<float>(mPopupWindow->getWidth()) / 2.0f;
                            auto cy = static_cast<float>(mPopupWindow->getHeight()) / 2.0f;
                            mPopupWindow->setTransform(
                                juce::AffineTransform::scale(progress, progress, cx, cy));
                        })
                        .build()),

      // --- 关闭动画器 (easeIn, 200ms) ---
      mCloseAnimator(juce::ValueAnimatorBuilder{}
                         .withDurationMs(200)
                         .withEasing(juce::Easings::createEaseIn())
                         .withValueChangedCallback([this](float progress)
                         {
                             float invProgress = 1.0f - progress;
                             mPopupWindow->setAlpha(invProgress);
                             auto cx = static_cast<float>(mPopupWindow->getWidth()) / 2.0f;
                             auto cy = static_cast<float>(mPopupWindow->getHeight()) / 2.0f;
                             mPopupWindow->setTransform(
                                 juce::AffineTransform::scale(invProgress, invProgress, cx, cy));
                         })
                         .withOnCompleteCallback([this]
                         {
                             mPopupWindow->setVisible(false);
                             mPopupWindow->setTransform(juce::AffineTransform());
                             if (auto* parent = mPopupWindow->getParentComponent())
                                 parent->removeChildComponent(mPopupWindow.get());
                         })
                         .build())
{
    mButton = button.get();
    mVBlankAnimatorUpdater->addAnimator(mOpenAnimator);
    mVBlankAnimatorUpdater->addAnimator(mCloseAnimator);

    mButton->setClickingTogglesState(true);

    // 注册监听
    mButton->addListener(this);
    mPopupWindow->xButton.addListener(this);

    addAndMakeVisible(mButton);
}

PopupWindowButton::~PopupWindowButton()
{
    if (auto* parent = mPopupWindow->getParentComponent())
        parent->removeChildComponent(mPopupWindow.get());

    if (mVBlankAnimatorUpdater != nullptr)
    {
        mVBlankAnimatorUpdater->removeAnimator(mOpenAnimator);
        mVBlankAnimatorUpdater->removeAnimator(mCloseAnimator);
    }
}

void PopupWindowButton::resized()
{
    mButton->setBounds(getLocalBounds());
}

void PopupWindowButton::paint(juce::Graphics&) {}

void PopupWindowButton::buttonClicked(juce::Button* button)
{
    if (button == mButton)
    {
        if (button->getToggleState())
        {
            if (!mIsPopupVisible)
                mOpenAnimator.start();
        }
        else
        {
            if (mIsPopupVisible)
            {
                mIsPopupVisible = false;
                mCloseAnimator.start();
            }
        }
    }
    if (button == &(mPopupWindow->xButton))
    {
        if (mIsPopupVisible)
        {
            mIsPopupVisible = false;
            mButton->setToggleState(false, juce::dontSendNotification);
            mCloseAnimator.start();
        }
    }
}

void PopupWindowButton::setPopupNoSee()
{
    if (!mIsPopupVisible)
        return;

    mIsPopupVisible = false;
    mButton->setToggleState(false, juce::dontSendNotification);
    mCloseAnimator.start();
}
