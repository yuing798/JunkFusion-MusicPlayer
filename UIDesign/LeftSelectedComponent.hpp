#pragma once

#include "FontAbout/language.h"
#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <array>
#include <cstddef>
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
    std::array<selectedButton, 12> buttons{
        selectedButton(U(allMusicID)),
        selectedButton(U(myLikeID)),
        selectedButton(U(recentPlayID)),
        selectedButton(U(artistID)),
        selectedButton(U(albumID)),
        selectedButton(U(playlistID)),
        selectedButton(U(genreID)),
        selectedButton(U(aiAssistantID)),
        selectedButton(U(effectsID)),
        selectedButton(U(equalizerID)),
        selectedButton(U(speakerArrayID)),
        selectedButton(U(settingsID))
    };//UI组件是不可拷贝且不可移动的，所以不能直接把实例塞入vector中
    std::array<YLabel,3> labelArray{
        YLabel(U(musicLibraryBrowseID)),
        YLabel(U(categoryBrowseID)),
        YLabel(U(featureSectionID))
    };
    std::array<juce::Path,6> paths;   

public:
    LeftSelectedComponent();
    ~LeftSelectedComponent() override;
    void resized() override;
    void paint(juce::Graphics& g) override;
    size_t height = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftSelectedComponent);
};

class LeftComponent : public juce::Component{
//这个东西就是主页面左边可以用来切换页面的侧边框，选择我喜欢的音乐，作者，那些乱七八糟的东西
private:
    LeftSelectedComponent mLeftSelectedComponent;
    juce::Viewport mViewPort;
public:
    LeftComponent();
    void resized() override;
    void paint(juce::Graphics& g) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftComponent);
};
