#include "popupWindow.hpp"
#include "buttons.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>
#include <utility>


// ======================================================================
// popupWindow 实现
// ======================================================================

popupWindow::popupWindow(juce::String title,std::unique_ptr<juce::Component> windowComponent)
    : mTitle(title),mWindowComponent(std::move(windowComponent))
{
    xButton.setClickingTogglesState(true);
    addAndMakeVisible(xButton);
    addAndMakeVisible(*mWindowComponent);

    // 总高度 = 标题行 + 间距 + 次组件的高度
    float titleRowHeight = mTitleFont.getHeight() + 10.0f;
    float totalHeight = titleRowHeight + mWindowComponent->getHeight();
    setSize(static_cast<int>(mWindowComponent->getWidth()), static_cast<int>(totalHeight));
}

void popupWindow::resized()
{
    auto bounds = getLocalBounds();
    float titleRowHeight = mTitleFont.getHeight() + 10.0f;

    // xButton — 右上角，正方形
    int buttonSize = static_cast<int>(titleRowHeight);
    auto titleRow = bounds.removeFromTop(buttonSize);
    xButton.setBounds(titleRow.removeFromRight(buttonSize).reduced(4));
    bounds.removeFromTop(10);
    mWindowComponent->setTopLeftPosition(bounds.getX(),bounds.getY());
}

void popupWindow::paint(juce::Graphics& g)
{
    auto local{getLocalBounds().toFloat()};

    // 背景与边框
    g.setColour(ycolor.shallowGrey);
    g.fillRoundedRectangle(local, 7.0f);

    float titleRowHeight = mTitleFont.getHeight() + 10.0f;

    // 标题 — 标题行内居中
    {
        juce::Rectangle<float> titleBounds(10.0f, 0.0f,
                                            mWindowComponent->getWidth() - 2.0f * 10.0f, titleRowHeight);
        g.setFont(mTitleFont);
        g.setColour(ycolor.black);
        g.drawText(mTitle, titleBounds, juce::Justification::centred, false);
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
    std::unique_ptr<juce::Button> button,
    juce::String windowTitle,
    std::unique_ptr<juce::Component> windowComponent
)
    : mButton(std::move(button)),
      mPopupWindow(std::make_unique<popupWindow>(windowTitle,std::move(windowComponent))),
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
    mVBlankAnimatorUpdater->addAnimator(mOpenAnimator);
    mVBlankAnimatorUpdater->addAnimator(mCloseAnimator);

    mButton->setClickingTogglesState(true);

    // 注册监听
    mButton->addListener(this);
    mPopupWindow->xButton.addListener(this);

    addAndMakeVisible(*mButton);
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
    if (button == mButton.get())
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
