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
    static constexpr float kAnimDurationMs    = 200.0;//动画持续时间为200ms

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
    std::unique_ptr<YComboPopup> mPopup{nullptr};

    class PopupDismissListener;
    std::unique_ptr<PopupDismissListener> mDismissListener;

    std::function<void (int)> onItemClicked;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YComboBox)
};