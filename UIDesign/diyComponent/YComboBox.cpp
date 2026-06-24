#include "YComboBox.hpp"
#include "otherComponent.hpp"
// ======================================================================
// YComboPopup — YComboBox 弹出菜单的内部组件
// ======================================================================
class YComboBox::YComboPopup : public juce::Component
{
public:
    YComboPopup (YComboBox& owner, const juce::StringArray& items)//管有它的comboBox和comboBox中注册了哪些字符串
        : mOwner (owner), mItems (items) {}

    void setHighlightedIndex (int index)//设置鼠标对应区域的高亮显示
    {
        if (mHighlightedIndex != index)
        {
            mHighlightedIndex = index;
            repaint();
        }
    }

    int getHighlightedIndex() const { return mHighlightedIndex; }//得到高亮显示区域的索引号
    int getItemCount() const        { return mItems.size(); }//得到现在有多少个注册了的字符串参数

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 背景
        g.setColour (ycolor.white);
        g.fillRoundedRectangle (bounds, YComboBox::kCornerRadius);

        // 边框 1px
        g.setColour (ycolor.shallowGrey);
        g.drawRoundedRectangle (bounds.reduced (0.5f),
                                YComboBox::kCornerRadius,
                                YComboBox::kPopupOutlineWidth);

        if (mItems.isEmpty())
            return;

        auto itemHeight = bounds.getHeight() / (float) mItems.size();
        g.setFont (juce::FontOptions (YComboBox::kFontSize));

        for (int i = 0; i < mItems.size(); ++i)
        {
            auto itemBounds = juce::Rectangle<float> (0.0f, i * itemHeight,
                                                       bounds.getWidth(), itemHeight);

            // 悬停高亮 — 圆角填充，无边框
            if (i == mHighlightedIndex)
            {
                g.setColour (ycolor.shallowGrey);
                g.fillRoundedRectangle (itemBounds.reduced (2.0f), YComboBox::kCornerRadius);
            }

            // 文字
            g.setColour (ycolor.black);
            g.drawText (mItems[i], itemBounds.reduced (8.0f, 0.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setHighlightedIndex (getItemIndexAtPosition (e.position));
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        int clickedIndex = getItemIndexAtPosition (e.position);
        if (clickedIndex >= 0 && clickedIndex < mItems.size())
        {
            mOwner.selectItem (clickedIndex);
            mOwner.hideMenu();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        setHighlightedIndex (-1);
    }

private:
    int getItemIndexAtPosition (juce::Point<float> pos) const
    {
        if (mItems.isEmpty())
            return -1;

        auto itemHeight = (float) getHeight() / (float) mItems.size();
        int index = (int) (pos.y / itemHeight);
        if (index < 0 || index >= mItems.size())
            return -1;
        return index;
    }

    YComboBox& mOwner;
    const juce::StringArray& mItems;
    int mHighlightedIndex = -1;
    // float mOpacity = 1.0f;
};

// ======================================================================
// PopupDismissListener — 点击弹出菜单外部时关闭
// ======================================================================
class YComboBox::PopupDismissListener : public juce::MouseListener
{
public:
    explicit PopupDismissListener (YComboBox& owner) : mOwner (owner) {}

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto* eventComp = e.eventComponent;
        // 如果点击的不是 YComboBox 本身也不是 popup → 关闭
        if (eventComp != &mOwner
            && mOwner.mPopup != nullptr
            && eventComp != mOwner.mPopup.get())
        {
            mOwner.hideMenu();
        }
    }

private:
    YComboBox& mOwner;
};
// ======================================================================
// YComboBox 实现
// ======================================================================

YComboBox::YComboBox()
    :mVBlankAnimatorUpdater(std::make_unique<juce::VBlankAnimatorUpdater> (this)),

    // --- 打开动画器 (easeOut, 200ms) ---
    mOpenAnimator(juce::ValueAnimatorBuilder{}
    .withDurationMs (kAnimDurationMs)
    .withEasing (juce::Easings::createEaseOut())//淡出
    .withOnStartCallback ([this]
    {
        mIsPopupVisible = true;
        createPopup();
    })
    .withValueChangedCallback ([this] (float progress)
    {
        mPopupOpacity = progress;
        updatePopupAppearance();
    })
    .build()),

    // --- 关闭动画器 (easeIn, 200ms) ---
    mCloseAnimator(juce::ValueAnimatorBuilder{}
    .withDurationMs (kAnimDurationMs)
    .withEasing (juce::Easings::createEaseIn())//淡入
    .withValueChangedCallback ([this] (float progress)
    {
        mPopupOpacity = 1.0f - progress;
        updatePopupAppearance();
    })
    .withOnCompleteCallback ([this]
    {
        removePopup();
    })
    .build())
{
    mVBlankAnimatorUpdater->addAnimator (mOpenAnimator);
    mVBlankAnimatorUpdater->addAnimator (mCloseAnimator);
}

YComboBox::~YComboBox()
{
    if (mDismissListener != nullptr)
        juce::Desktop::getInstance().removeGlobalMouseListener (mDismissListener.get());

    mPopup.reset();
    mDismissListener.reset();

    if (mVBlankAnimatorUpdater != nullptr)
    {
        mVBlankAnimatorUpdater->removeAnimator (mOpenAnimator);
        mVBlankAnimatorUpdater->removeAnimator (mCloseAnimator);
    }
}
// ----------------------------------------------------------------------
// 弹出菜单生命周期
// ----------------------------------------------------------------------
void YComboBox::createPopup()
{
    mPopup = std::make_unique<YComboPopup> (*this, mItems);

    if (auto* tl = getTopLevelComponent()) tl->addAndMakeVisible (*mPopup);
    //注意这个YComboPopup是属于主窗口的

    updatePopupAppearance();

    // 安装全局鼠标监听以支持点击外部关闭
    if (mDismissListener == nullptr)
    {
        mDismissListener = std::make_unique<PopupDismissListener> (*this);
        juce::Desktop::getInstance().addGlobalMouseListener (mDismissListener.get());
        //注册为全局鼠标监听器，之后整个应用程序中发生的所有鼠标事件，这个对象都会收到通知
    }
}

void YComboBox::removePopup()
{
    if (mDismissListener != nullptr)
    {
        juce::Desktop::getInstance().removeGlobalMouseListener (mDismissListener.get());
        mDismissListener.reset();
    }

    if (mPopup != nullptr)
    {
        if (auto* p = mPopup->getParentComponent())
            p->removeChildComponent (mPopup.get());
        mPopup.reset();
    }
}

void YComboBox::updatePopupAppearance()
{
    if (mPopup == nullptr)
        return;
    
    auto thisPos = getTopLevelComponent()->getLocalPoint(this, juce::Point<int>(0, 0));
    //获取当前组件在整个窗口的绝对坐标
    //将当前组件左上角 (0,0) 这个点，转换到顶层组件的本地坐标系中，从而得到当前组件在顶层组件（即主窗口）内的相对位置

    int popupX   = thisPos.getX();
    int popupY   = thisPos.getY() + getHeight();
    int popupW   = getWidth();
    int popupH   = getHeight() * mItems.size();

    mPopup->setBounds (popupX, popupY, popupW, popupH);
    mPopup->setAlpha(mPopupOpacity);
}

// ----------------------------------------------------------------------
// 显示 / 隐藏
// ----------------------------------------------------------------------
void YComboBox::showMenu (juce::Rectangle<int> /*targetBounds*/)
{
    if (mIsPopupVisible)
        return;

    mOpenAnimator.start();
}

void YComboBox::hideMenu()
{
    if (!mIsPopupVisible)
        return;

    mIsPopupVisible = false;
    mCloseAnimator.start();
}

// ----------------------------------------------------------------------
// 内部选择（由 popup 调用）
// ----------------------------------------------------------------------
void YComboBox::selectItem (int index)
{
    if (index >= 0 && index < mItems.size())
    {
        mSelectedIndex = index;
        repaint();

        if (onItemClicked)
            onItemClicked (index);
    }
}

// ----------------------------------------------------------------------
// 数据管理
// ----------------------------------------------------------------------
void YComboBox::addItem (const juce::String& itemText)
{
    mItems.add (itemText);
    if (mSelectedIndex < 0)
        mSelectedIndex = 0;
}

void YComboBox::addItems (const juce::StringArray& items)
{
    mItems.addArray (items);
    if (mSelectedIndex < 0 && !mItems.isEmpty())
        mSelectedIndex = 0;
}

void YComboBox::addChineseItems (std::vector<const char*> items)
{
    for (auto* ch : items)
        mItems.add (juce::String (juce::CharPointer_UTF8 (ch)));
    if (mSelectedIndex < 0 && !mItems.isEmpty())
        mSelectedIndex = 0;
}

void YComboBox::clearItems()
{
    mItems.clear();
    mSelectedIndex = -1;
    repaint();
}

int YComboBox::getNumItems() const
{
    return mItems.size();
}

juce::String YComboBox::getItemText (int index) const
{
    if (index >= 0 && index < mItems.size())
        return mItems[index];
    return {};
}

// ----------------------------------------------------------------------
// 选择管理
// ----------------------------------------------------------------------
void YComboBox::setSelectedItemIndex (int index, juce::NotificationType notification)
{
    if (index >= 0 && index < mItems.size())
    {
        mSelectedIndex = index;
        repaint();

        if (notification == juce::sendNotification && onItemClicked)
            onItemClicked (index);
    }
}

int YComboBox::getSelectedItemIndex() const
{
    return mSelectedIndex;
}

juce::String YComboBox::getSelectedItemText() const
{
    return getItemText (mSelectedIndex);
}

// ----------------------------------------------------------------------
// 回调
// ----------------------------------------------------------------------
void YComboBox::onItemSelected (std::function<void (int)> callback)
{
    onItemClicked = std::move (callback);
}

// ----------------------------------------------------------------------
// 布局
// ----------------------------------------------------------------------
void YComboBox::resized()
{
    // 如果 popup 可见，更新其位置
    if (mPopup != nullptr)
        updatePopupAppearance();
}

// ----------------------------------------------------------------------
// 绘制 — 主下拉框
// ----------------------------------------------------------------------
void YComboBox::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. 白色背景
    g.setColour (ycolor.white);
    g.fillRoundedRectangle (bounds, kCornerRadius);

    // 2. 3px shallowGrey 圆角边框
    g.setColour (ycolor.shallowGrey);
    g.drawRoundedRectangle (bounds.reduced (kOutlineWidth * 0.5f),
                            kCornerRadius, kOutlineWidth);

    // 3. 分离文字区域和箭头区域
    g.setFont (juce::FontOptions (kFontSize));
    auto arrowArea = bounds.removeFromRight ((float) getHeight());
    auto textArea  = bounds.reduced (10.0f, 0.0f);

    // 4. 选中项文字
    if (mSelectedIndex >= 0 && mSelectedIndex < mItems.size())
    {
        g.setColour (ycolor.black);
        g.drawText (mItems[mSelectedIndex], textArea,
                    juce::Justification::centredLeft, false);
    }

    // 5. 倒三角箭头
    float arrowCx = arrowArea.getCentreX();
    float arrowCy = arrowArea.getCentreY();

    juce::Path arrowPath;
    arrowPath.addTriangle (
        arrowCx,                     arrowCy + kArrowSize * 0.6f,   // 底点（朝下）
        arrowCx - kArrowSize,         arrowCy - kArrowSize * 0.4f, // 左上
        arrowCx + kArrowSize,         arrowCy - kArrowSize * 0.4f  // 右上
    );

    g.setColour (mIsHoveringArrow ? ycolor.shallowGrey : ycolor.black);
    g.fillPath (arrowPath);
}

// ----------------------------------------------------------------------
// 鼠标事件
// ----------------------------------------------------------------------
void YComboBox::mouseDown (const juce::MouseEvent& e)
{
    if (mIsPopupVisible)
    {
        hideMenu();
    }
    else
    {
        showMenu (getScreenBounds());
    }
}

void YComboBox::mouseMove (const juce::MouseEvent& e)
{
    auto arrowZone = getLocalBounds().removeFromRight (getHeight());
    bool overArrow = arrowZone.contains (e.getPosition());

    if (mIsHoveringArrow != overArrow)
    {
        mIsHoveringArrow = overArrow;
        repaint();
    }
}

void YComboBox::mouseExit (const juce::MouseEvent&)
{
    if (mIsHoveringArrow)
    {
        mIsHoveringArrow = false;
        repaint();
    }
}