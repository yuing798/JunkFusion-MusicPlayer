#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "./diyComponent/otherComponent.hpp"
#include "./diyComponent/YComboBox.hpp"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <filesystem>
#include <memory>
#include <vector>
#include "./diyComponent/buttons.hpp"
#include "./diyComponent/popupWindow.hpp"

class PageChange : public juce::Component{
//页码切换的组件
private:
    std::vector<std::unique_ptr<yTextButton>> Buttons;
    //按钮点击切换
    //索引0为第一页，最后一个索引为最后一页，应该只显示出当前页码的前后共五页
    yTextButton popButton{U("跳转")};
    juce::TextEditor mTextEditor;
public:
    void resized() override;
    PageChange();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageChange)
};


class AllMusicComponent : public juce::Component{
private:

public:
    AllMusicComponent();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicComponent)
};

class allMusicPage : public juce::Component,
                     public juce::Button::Listener
{
private:

    AllMusicComponent mAllMusicComponent;
    yTextButton selectFileButton{U("导入文件")};
    BigLabel allMusicLabel{U("全部音乐")};
    svgButton refreshButton{
        U("刷新"),
        juce::Drawable::createFromImageData(BinaryData::refresh_svg, BinaryData::refresh_svgSize)
    };
    littleLabel numSongsLabel{U("共 0 首")};

    //搜索框
    juce::Viewport mViewPort;
    upDownButton mUpDownButton;
    yTextButton playAll{U("播放全部")};

public:

    allMusicPage();
    void resized() override;
    void buttonClicked(juce::Button*) override;
    // void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(allMusicPage)
};
