#include "MainComponent.h"
#include "UIDesign/diyComponent/lookandfeel.hpp"
#include "UIDesign/diyComponent/otherComponent.hpp"
#include "juce_gui_basics/juce_gui_basics.h"
#include "serial.hpp"

//==============================================================================
MainComponent::MainComponent()
:tooltipWindow(this,400){
    setLookAndFeel(&mLook);

    // ── 左侧栏 ──
    addAndMakeVisible(mLeftComponent);

    // ── 注册所有页面 ──
    addChildComponent(mAllMusicPage);     // 0: 所有音乐（默认可见）
    addChildComponent(mMyLikePage);       // 1: 我喜欢
    addChildComponent(mRecentPlayPage);   // 2: 最近播放
    addChildComponent(mArtistPage);       // 3: 作者
    addChildComponent(mAlbumPage);        // 4: 专辑
    addChildComponent(mPlaylistPage);     // 5: 歌单
    addChildComponent(mGenrePage);        // 6: 风格
    addChildComponent(mAiAssistantPage);  // 7: AI助手
    addChildComponent(mEffectsPage);      // 8: 效果器
    addChildComponent(mEqualizerPage);    // 9: 均衡器
    addChildComponent(mSpeakerArrayPage); // 10: 音箱阵列
    addChildComponent(mSettingsPage);     // 11: 设置

    mLeftComponent.getLeftSelectedComponent().setOnSelectionChanged([this](int id)
    {
        selectID = id;
        switchPage(id);
    });

    // 默认显示"所有音乐"
    mAllMusicPage.setVisible(true);

    setSize (1400, 700);
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.setColour(ycolor.white);
    g.fillAll();
}

void MainComponent::resized()
{
    auto local = getLocalBounds();
    auto height = local.getHeight();
    auto width  = local.getWidth();

    // 左侧栏占 180px
    mLeftComponent.setBounds(local.removeFromLeft(180));

    // 当前可见页面填充剩余区域
    juce::Component* pages[] = {
        &mAllMusicPage,       // 0
        &mMyLikePage,         // 1
        &mRecentPlayPage,     // 2
        &mArtistPage,         // 3
        &mAlbumPage,          // 4
        &mPlaylistPage,       // 5
        &mGenrePage,          // 6
        &mAiAssistantPage,    // 7
        &mEffectsPage,        // 8
        &mEqualizerPage,      // 9
        &mSpeakerArrayPage,   // 10
        &mSettingsPage        // 11
    };

    pages[selectID]->setBounds(local);
}

void MainComponent::switchPage(int id)
{
    // 先全部隐藏
    mAllMusicPage     .setVisible(false);
    mMyLikePage       .setVisible(false);
    mRecentPlayPage   .setVisible(false);
    mArtistPage       .setVisible(false);
    mAlbumPage        .setVisible(false);
    mPlaylistPage     .setVisible(false);
    mGenrePage        .setVisible(false);
    mAiAssistantPage  .setVisible(false);
    mEffectsPage      .setVisible(false);
    mEqualizerPage    .setVisible(false);
    mSpeakerArrayPage .setVisible(false);
    mSettingsPage     .setVisible(false);

    // 根据 id 显示对应页面
    switch (id)
    {
        case 0:  mAllMusicPage     .setVisible(true); break; // 所有音乐
        case 1:  mMyLikePage       .setVisible(true); break; // 我喜欢
        case 2:  mRecentPlayPage   .setVisible(true); break; // 最近播放
        case 3:  mArtistPage       .setVisible(true); break; // 作者
        case 4:  mAlbumPage        .setVisible(true); break; // 专辑
        case 5:  mPlaylistPage     .setVisible(true); break; // 歌单
        case 6:  mGenrePage        .setVisible(true); break; // 风格
        case 7:  mAiAssistantPage  .setVisible(true); break; // AI助手
        case 8:  mEffectsPage      .setVisible(true); break; // 效果器
        case 9:  mEqualizerPage    .setVisible(true); break; // 均衡器
        case 10: mSpeakerArrayPage .setVisible(true); break; // 音箱阵列
        case 11: mSettingsPage     .setVisible(true); break; // 设置
        default: break;
    }

    // 触发重新布局
    resized();
}

bool MainComponent::keyPressed(const juce::KeyPress& key){
    if(key==juce::KeyPress('s',juce::ModifierKeys::commandModifier,0)){
        //序列化保存逻辑
        Serial::getInstance().stopTimer();
        Serial::getInstance().save2disk();
        Serial::getInstance().startTimer(60000);
    }
}

MainComponent::~MainComponent(){
    setLookAndFeel(nullptr);
}
