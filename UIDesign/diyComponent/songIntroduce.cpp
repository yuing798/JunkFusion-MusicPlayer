#include "songIntroduce.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

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

    // 总高度 = 标题行 + 间距 + 图片 + 间距 + 文本 + 间距
    float titleRowHeight = mTitleFont.getHeight() + kPadding;
    float totalHeight = titleRowHeight + kPadding + kImageSize + kPadding + mTextHeight + kPadding;
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
    xButton.setBounds(titleRow.removeFromRight(buttonSize));
}

void popupWindow::paint(juce::Graphics& g)
{
    auto local{getLocalBounds().toFloat()};

    // 背景与边框
    g.setColour(ycolor.white);
    g.fillRoundedRectangle(local, 4.0f);
    g.setColour(ycolor.darkGrey);
    g.drawRoundedRectangle(0, 0, local.getWidth(), local.getHeight(), 4.0f, 3.0f);

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

    // 图片 — 200×200 居中
    {
        float imageX = (kPopupWidth - kImageSize) / 2.0f;
        juce::Rectangle<float> imageBounds(imageX, currentY, kImageSize, kImageSize);
        if (mImage.isValid())
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
// SongIntroduce 实现
// ======================================================================

SongIntroduce::SongIntroduce(juce::Image image, juce::String text, juce::String title)
    : mVBlankAnimatorUpdater(std::make_unique<juce::VBlankAnimatorUpdater>(this)),
      mPopupWindow(image, text, title),

      // --- 打开动画器 (easeOut, 200ms) ---
      mOpenAnimator(juce::ValueAnimatorBuilder{}
                        .withDurationMs(200)
                        .withEasing(juce::Easings::createEaseOut())
                        .withOnStartCallback([this]
                        {
                            // 延迟到动画启动时才添加到顶层组件，确保组件树已建立
                            auto* topLevel = getTopLevelComponent();
                            if (mPopupWindow.getParentComponent() != topLevel)
                            {
                                if (auto* oldParent = mPopupWindow.getParentComponent())
                                    oldParent->removeChildComponent(&mPopupWindow);
                                topLevel->addChildComponent(mPopupWindow);
                            }
                            mIsPopupVisible = true;
                            mPopupWindow.resetPosition();
                            mPopupWindow.setAlpha(0.0f);
                            mPopupWindow.setVisible(true);
                            mPopupWindow.xButton.setToggleState(false, juce::dontSendNotification);
                        })
                        .withValueChangedCallback([this](float progress)
                        {
                            mPopupWindow.setAlpha(progress);
                            auto cx = static_cast<float>(mPopupWindow.getWidth()) / 2.0f;
                            auto cy = static_cast<float>(mPopupWindow.getHeight()) / 2.0f;
                            mPopupWindow.setTransform(
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
                             mPopupWindow.setAlpha(invProgress);
                             auto cx = static_cast<float>(mPopupWindow.getWidth()) / 2.0f;
                             auto cy = static_cast<float>(mPopupWindow.getHeight()) / 2.0f;
                             mPopupWindow.setTransform(
                                 juce::AffineTransform::scale(invProgress, invProgress, cx, cy));
                         })
                         .withOnCompleteCallback([this]
                         {
                             mPopupWindow.setVisible(false);
                             mPopupWindow.setTransform(juce::AffineTransform());
                             if (auto* parent = mPopupWindow.getParentComponent())
                                 parent->removeChildComponent(&mPopupWindow);
                         })
                         .build())
{
    mVBlankAnimatorUpdater->addAnimator(mOpenAnimator);
    mVBlankAnimatorUpdater->addAnimator(mCloseAnimator);

    whatsmoreButton.setClickingTogglesState(true);

    // 注册监听
    whatsmoreButton.addListener(this);
    mPopupWindow.xButton.addListener(this);

    addAndMakeVisible(whatsmoreButton);
}

SongIntroduce::~SongIntroduce()
{
    if (auto* parent = mPopupWindow.getParentComponent())
        parent->removeChildComponent(&mPopupWindow);

    if (mVBlankAnimatorUpdater != nullptr)
    {
        mVBlankAnimatorUpdater->removeAnimator(mOpenAnimator);
        mVBlankAnimatorUpdater->removeAnimator(mCloseAnimator);
    }
}

void SongIntroduce::resized()
{
    whatsmoreButton.setBounds(getLocalBounds());
}

void SongIntroduce::paint(juce::Graphics&) {}

void SongIntroduce::buttonClicked(juce::Button* button)
{
    if (button == &whatsmoreButton)
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
    if (button == &mPopupWindow.xButton)
    {
        if (mIsPopupVisible)
        {
            mIsPopupVisible = false;
            whatsmoreButton.setToggleState(false, juce::dontSendNotification);
            mCloseAnimator.start();
        }
    }
}

void SongIntroduce::setPopupNoSee()
{
    if (!mIsPopupVisible)
        return;

    mIsPopupVisible = false;
    whatsmoreButton.setToggleState(false, juce::dontSendNotification);
    mCloseAnimator.start();
}
