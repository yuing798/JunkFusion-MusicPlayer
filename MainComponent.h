#pragma once

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include "UIDesign/LeftColumn.hpp"
#include "UIDesign/Pages/allMusicPage.hpp"
#include "UISet.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent final : public juce::Component
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...
    YLookAndFeel mLook;
    LeftColumn mLeftComponent;
    int selectID{0};

    // ── 页面组件 ──
    // 0: 所有音乐
    allMusicPage mAllMusicPage;
    // 1: 我喜欢
    juce::Component mMyLikePage;
    // 2: 最近播放
    juce::Component mRecentPlayPage;
    // 3: 作者
    juce::Component mArtistPage;
    // 4: 专辑
    juce::Component mAlbumPage;
    // 5: 歌单
    juce::Component mPlaylistPage;
    // 6: 风格
    juce::Component mGenrePage;
    // 7: AI助手
    juce::Component mAiAssistantPage;
    // 8: 效果器
    juce::Component mEffectsPage;
    // 9: 均衡器
    juce::Component mEqualizerPage;
    // 10: 音箱阵列
    juce::Component mSpeakerArrayPage;
    // 11: 设置
    juce::Component mSettingsPage;

    void switchPage(int id);

    juce::TooltipWindow tooltipWindow;//悬停说明

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
