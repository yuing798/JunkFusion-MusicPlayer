#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "./diyComponent/otherComponent.hpp"
#include "./diyComponent/YComboBox.hpp"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <filesystem>
#include <memory>
#include <vector>
#include "./diyComponent/buttons.hpp"
#include "./diyComponent/popupWindow.hpp"


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
    YLabel sortWayLabel{U("排列方法")};
    YComboBox sortWaysComboBox;

    //搜索框
    juce::Viewport mViewPort;
    upDownButton mUpDownButton;
    yTextButton playAll{U("播放全部")};
    //这个是用来测试的
    std::unique_ptr<PopupWindowButton> mPopupWindowButton;

public:

    allMusicPage();
    void resized() override;
    void buttonClicked(juce::Button*) override;
    // void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(allMusicPage)
};
