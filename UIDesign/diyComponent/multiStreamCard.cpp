#include "multiStreamCard.hpp"
#include "FontAbout/font.h"
#include "juce_events/juce_events.h"
#include "otherComponent.hpp"
#include "multiStreamCard.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

EachStream::EachStream(SongInfo::stream& stream){
    addAndMakeVisible(mPlayStopButton);
    addAndMakeVisible(mCountLabel);
    addAndMakeVisible(mNumChannels);
    addAndMakeVisible(mBitDepth);
    addAndMakeVisible(mBitRate);
    addAndMakeVisible(mDecoderName);
    addAndMakeVisible(*mEllipsisToolTip);

    mCountLabel.setText(juce::String(stream.streamCount), dontSendNotification);
    mNumChannels.setText(juce::String(stream.numChannels), dontSendNotification);
    mBitDepth.setText(juce::String(stream.bitDepth), dontSendNotification);
    mBitRate.setText(juce::String(stream.bitRate), dontSendNotification);
    mDecoderName.setText(juce::String(stream.codecName), dontSendNotification);

    juce::String isMusic = stream.isMusic ? U("是") : U("否");
    juce::String isAIprocessed = stream.aiProcessed ? U("是") : U("否");

    mEllipsisToolTip = std::make_unique<EllipsisToolTip>(longUTF8(
        "AI分析体裁:",stream.aiGenre,
        "\nAI分析情绪:",stream.aiMood,
        "\n是否已经进行过AI分析:",isAIprocessed,
        "\nBPM:",juce::String(stream.bpm),
        "\n是否为音乐资源",isMusic,
        "\n调性:",juce::String(stream.key),
        "\n采样率:",juce::String(stream.sampleRate),
        "\n额外信息",stream.extraMetadata
    ));

    setSize(700, 100);
}
void EachStream::resized(){
    auto local{getLocalBounds()};
    mPlayStopButton.setBounds(local.removeFromLeft(getHeight()).reduced(30));
    mCountLabel.setBounds(local.removeFromLeft(50).reduced(8));
    mNumChannels.setBounds(local.removeFromLeft(50).reduced(8));
    mBitDepth.setBounds(local.removeFromLeft(100).reduced(10));
    mBitRate.setBounds(local.removeFromLeft(100).reduced(10));
    mDecoderName.setBounds(local.removeFromLeft(200).reduced(10));
    mEllipsisToolTip->setBounds(local.removeFromLeft(getHeight()).reduced(30));
}
void EachStream::paint(juce::Graphics& g){
    auto local{getLocalBounds().toFloat()};
    g.setColour(ycolor.shallowGrey);
    g.fillRoundedRectangle(local,15.0f);
}

// ======================================================================
// MultiStreamPopupWindow 实现
// ======================================================================

MultiStreamPopupWindow::MultiStreamPopupWindow(juce::Image image, juce::String text, juce::String title)
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

MultiStreamPopupWindow::~MultiStreamPopupWindow() {}

void MultiStreamPopupWindow::resized()
{
    auto bounds = getLocalBounds();
    float titleRowHeight = mTitleFont.getHeight() + kPadding;

    // xButton — 右上角，正方形
    int buttonSize = static_cast<int>(titleRowHeight);
    auto titleRow = bounds.removeFromTop(buttonSize);
    xButton.setBounds(titleRow.removeFromRight(buttonSize).reduced(4));
}

void MultiStreamPopupWindow::paint(juce::Graphics& g)
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

void MultiStreamPopupWindow::mouseDown(const juce::MouseEvent& e)
{
    mDragger.startDraggingComponent(this, e);
}

void MultiStreamPopupWindow::mouseDrag(const juce::MouseEvent& e)
{
    mDragger.dragComponent(this, e, nullptr);
}

void MultiStreamPopupWindow::resetPosition()
{
    if (auto* parent = getParentComponent())
        setCentrePosition(parent->getLocalBounds().getCentre());
}

// ======================================================================
// MultiStreamCard 实现
// ======================================================================

MultiStreamCardButton::MultiStreamCardButton(){
    setButtonText(U("多流音频"));
    setClickingTogglesState(true);
}
void MultiStreamCardButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown){
    auto local{getLocalBounds().toFloat()};
    g.setColour(ycolor.greyBlue);
    g.fillRoundedRectangle(local,3.0f);
    g.setColour(ycolor.white);
    g.setFont(juce::FontOptions{}.withHeight(16.0f));
    g.drawText(getButtonText(),local,juce::Justification::centred,true);
}

MultiStreamCard::MultiStreamCard(juce::Image image, juce::String text, juce::String title)
    : 
      mPopupWindow(image, text, title),
      mVBlankAnimatorUpdater(std::make_unique<juce::VBlankAnimatorUpdater>(this)),

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

    cardButton.setClickingTogglesState(true);

    // 注册监听
    cardButton.addListener(this);
    mPopupWindow.xButton.addListener(this);

    addAndMakeVisible(cardButton);
}

MultiStreamCard::~MultiStreamCard()
{
    if (auto* parent = mPopupWindow.getParentComponent())
        parent->removeChildComponent(&mPopupWindow);

    if (mVBlankAnimatorUpdater != nullptr)
    {
        mVBlankAnimatorUpdater->removeAnimator(mOpenAnimator);
        mVBlankAnimatorUpdater->removeAnimator(mCloseAnimator);
    }
}

void MultiStreamCard::resized()
{
    cardButton.setBounds(getLocalBounds());
}

void MultiStreamCard::paint(juce::Graphics&) {}

void MultiStreamCard::buttonClicked(juce::Button* button)
{
    if (button == &cardButton)
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
            cardButton.setToggleState(false, juce::dontSendNotification);
            mCloseAnimator.start();
        }
    }
}

void MultiStreamCard::setPopupNoSee()
{
    if (!mIsPopupVisible)
        return;

    mIsPopupVisible = false;
    cardButton.setToggleState(false, juce::dontSendNotification);
    mCloseAnimator.start();
}
