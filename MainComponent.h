#pragma once
#include "UIDesign/LeftColumn.hpp"
#include "UIDesign/Pages/allMusicPage.hpp"
#include <juce_gui_extra/juce_gui_extra.h>
#include "UIDesign/diyComponent/lookandfeel.hpp"
#include "UIDesign/diyComponent/otherComponent.hpp"

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
