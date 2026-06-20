#pragma once

#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_gui_basics/juce_gui_basics.h"

class AllMusicComponent : public juce::Component{
private:

public:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicComponent)
};

class allMusicPage : public juce::Component{
private:
    AllMusicComponent mAllMusicComponent;
    yTextButton selectFileButton{U("导入文件")};
    YLabel allMusicLabel{U("全部音乐")};
    
    //搜索框
public:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(allMusicPage)
};