#pragma once

#include "FontAbout/language.h"
#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>
#include <vector>

struct selectedButton : public juce::TextButton{//这个按钮点击了颜色会发生变化，但是文字不会改变
    selectedButton(juce::String initText);
    void paintButton (juce::Graphics& g, 
        bool shouldDrawButtonAsHighlighted, 
        bool shouldDrawButtonAsDown) override;
};

class LeftSelectedComponent : public juce::Component{

//这个是滑动窗口里面的内容，还没有和外层的侧边栏绑定
private:
    juce::OwnedArray<selectedButton> buttons;
    juce::OwnedArray<YLabel> labelArray;
    juce::OwnedArray<juce::Path> paths;

    // ── 曲库浏览 ──
    YLabel musicLibraryBrowse{U(musicLibraryBrowseID)};
    selectedButton allMusic           {U(allMusicID)};
    selectedButton myLike             {U(myLikeID)};
    selectedButton recentPlay         {U(recentPlayID)};

    // ── 分类浏览 ──
    YLabel categoryBrowse{U(categoryBrowseID)};
    selectedButton artist        {U(artistID)};
    selectedButton album         {U(albumID)};
    selectedButton playlist      {U(playlistID)};
    selectedButton genre         {U(genreID)};

    // ── 功能板块 ──
    YLabel featureSection{U(featureSectionID)};
    selectedButton aiAssistant   {U(aiAssistantID)};
    selectedButton effects       {U(effectsID)};
    selectedButton equalizer     {U(equalizerID)};
    selectedButton speakerArray  {U(speakerArrayID)};
    selectedButton settings      {U(settingsID)};

public:
    LeftSelectedComponent();
    void resized() override;
    void paint(juce::Graphics& g) override;
    int height;
};

class LeftSComponent : public juce::Component{
//这个东西就是主页面左边可以用来切换页面的侧边框，选择我喜欢的音乐，作者，那些乱七八糟的东西
private:
    LeftSelectedComponent mLeftSelectedComponent;
    juce::Viewport mViewPort;
public:
    LeftSComponent();
};
