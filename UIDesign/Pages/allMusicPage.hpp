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
    yTextButton popButton{U("跳转")};
    juce::TextEditor mTextEditor;
    yTextButton previousButton{U("上一页")};
    yTextButton nextButton{U("下一页")};
    yTextButton button1{U("1")};//第一页
    yTextButton button_1;//最后一页
    YLabel ellipsisLabel{U("...")};
public:
    void resized() override;
    PageChange();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageChange)
};


class AllMusicComponent : public juce::Component{
private:
    PageChange mPageChange;
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
    YComboBox seqWays;//排序方法
    juce::String selectedSeqWays;//当前选择的排列方法

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
