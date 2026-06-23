#pragma once
#include "FontAbout/font.h"
#include "constants.h"
#include "juce_animation/juce_animation.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>
#include <memory>

struct YColor{
    juce::Colour black = juce::Colours::black;
    juce::Colour white {0xfff9f9f9};//"#f9f9f9"这个不是纯白色，因为纯白色太亮了
    juce::Colour shallowGrey {0xffd8d6d6};//"#d8d6d6"
    juce::Colour midGrey {0xffc5c3c3};//"#c5c3c3"
    juce::Colour darkGrey{0xffababab};//"#ababab"
    juce::Colour blackGrey{0xff706f6f};//"#706f6f"
    juce::Colour greyBlue {0xff6e75dd};//"#6e75dd"
    juce::Colour greyGreen {0xff33c4de};//"#33c4de"
    juce::Colour transparent {0x00000000};//"#000000"
    //注意juce的透明度要放在首位
};

extern YColor ycolor;

struct YLabel : public juce::Label
{

    YLabel();
    YLabel(juce::String text);
};
struct littleLabel : juce::Label{
    
    littleLabel();
    littleLabel(juce::String text);
};//字号比较小一点的标签
struct BigLabel : juce::Label{
    
    BigLabel();
    BigLabel(juce::String text);
};//大大大字号标签

class YSlider : public juce::Slider{
public:
    YSlider();
};




struct rotarySlider : public juce::Slider{

    rotarySlider();
    
};

struct verticalSlider : public juce::Slider{
    verticalSlider();
};
struct WhatsMoreToolTip : juce::DrawableButton{
//一个省略号图形，悬停显示注释
    WhatsMoreToolTip(const juce::String& text);
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    std::unique_ptr<juce::Drawable> svg;
};

class YComboBox : public juce::Component
{
public:
    YComboBox ();
    ~YComboBox() override;

    // === 数据管理 ===
    void addItem (const juce::String& itemText);
    void addItems (const juce::StringArray& items);
    void addChineseItems (std::vector<const char*> items);
    void clearItems();
    int  getNumItems() const;
    juce::String getItemText (int index) const;

    // === 选择管理 ===
    void setSelectedItemIndex (int index, juce::NotificationType notification = juce::dontSendNotification);
    int  getSelectedItemIndex() const;
    juce::String getSelectedItemText() const;

    // === 回调 ===
    void onItemSelected (std::function<void (int)> callback);

    // === 弹出/收回（由内部动画驱动） ===
    void showMenu (juce::Rectangle<int> targetBounds);
    void hideMenu();

    // === juce::Component ===
    void resized() override;
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

private:
    // --- 内部方法 ---
    void buildAnimators();
    void createPopup();
    void removePopup();
    void updatePopupAppearance();
    void selectItem (int index);   // 由 popup 调用，触发回调 + repaint

    // --- 常量 ---
    static constexpr float kCornerRadius      = 6.0f;
    static constexpr float kOutlineWidth      = 3.0f;
    static constexpr float kPopupOutlineWidth = 1.0f;
    static constexpr float kArrowSize         = 8.0f;
    static constexpr float kFontSize          = 17.0f;
    static constexpr float kAnimDurationMs    = 200.0;

    // --- 成员 ---
    juce::StringArray mItems;
    int mSelectedIndex    = -1;
    bool mIsPopupVisible  = false;
    bool mIsHoveringArrow = false;

    std::unique_ptr<juce::VBlankAnimatorUpdater> mVBlankAnimatorUpdater;
    juce::Animator mOpenAnimator;
    juce::Animator mCloseAnimator;
    float mPopupOpacity = 0.0f;

    class YComboPopup;
    std::unique_ptr<YComboPopup> mPopup;

    class PopupDismissListener;
    std::unique_ptr<PopupDismissListener> mDismissListener;

    std::function<void (int)> onItemClicked;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YComboBox)
};