#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "UISet.h"
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
    std::unique_ptr<juce::Drawable> refreshSvg{
        juce::Drawable::createFromImageData(BinaryData::refresh_svg, BinaryData::refresh_svgSize)
    };
    //搜索框
    juce::Viewport mViewPort;
    
public:

    allMusicPage();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(allMusicPage)
};