#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"

class AllMusicComponent : public juce::Component{
private:

public:
    AllMusicComponent();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicComponent)
};

class allMusicPage : public juce::Component{
private:
    AllMusicComponent mAllMusicComponent;
    yTextButton selectFileButton{U("导入文件")};
    YLabel allMusicLabel{U("全部音乐")};
    svgButton refreshButton{
        U("刷新"),
        juce::Drawable::createFromImageData(BinaryData::refresh_svg, BinaryData::refresh_svgSize)
    };
    littleLabel numSongsLabel{U("共 0 首")};
    YLabel sortWayLabel{U("排列方法")};
    yComboBox sortWaysComboBox;

    //搜索框
    juce::Viewport mViewPort;
    doubleSvgButton upAndDownButton{
        U("升序"),
        juce::Drawable::createFromImageData(BinaryData::up_svg, BinaryData::up_svgSize),
        U("降序"),
        juce::Drawable::createFromImageData(BinaryData::down_svg, BinaryData::down_svgSize)
    };
    

public:

    allMusicPage();
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(allMusicPage)
};