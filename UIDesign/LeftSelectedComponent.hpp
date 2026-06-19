#pragma once

#include "FontAbout/language.h"
#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

struct selectedButton : public juce::TextButton{//这个按钮点击了颜色会发生变化，但是文字不会改变
    selectedButton(juce::String initText);
    void paintButton (juce::Graphics& g, 
        bool shouldDrawButtonAsHighlighted, 
        bool shouldDrawButtonAsDown) override;
};

class LeftSelectedComponent : public juce::Component{
//这个东西就是主页面左边可以用来切换页面的侧边框，选择我喜欢的音乐，作者，那些乱七八糟的东西
private:
    juce::Viewport mViewPort;//滑动窗口
    juce::Component mContentComponent;//Viewport 的内容组件
    std::vector<selectedButton*> buttons;

    // ── 曲库浏览 ──
    // selectedButton musicLibraryBrowse{U(musicLibraryBrowseID)};
    selectedButton allMusic           {U(allMusicID)};
    selectedButton myLike             {U(myLikeID)};
    selectedButton recentPlay         {U(recentPlayID)};

    // ── 分类浏览 ──
    // selectedButton categoryBrowse{U(categoryBrowseID)};
    selectedButton artist        {U(artistID)};
    selectedButton album         {U(albumID)};
    selectedButton playlist      {U(playlistID)};
    selectedButton genre         {U(genreID)};

    // ── 功能板块 ──
    // selectedButton featureSection{U(featureSectionID)};
    selectedButton aiAssistant   {U(aiAssistantID)};
    selectedButton effects       {U(effectsID)};
    selectedButton equalizer     {U(equalizerID)};
    selectedButton speakerArray  {U(speakerArrayID)};
    selectedButton settings      {U(settingsID)};

public:
    LeftSelectedComponent();
    void resized() override;
    void paint(juce::Graphics& g) override;
};
