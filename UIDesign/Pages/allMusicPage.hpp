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
#include <array>
#include <filesystem>
#include <memory>
#include <vector>
#include "./diyComponent/buttons.hpp"
#include "./diyComponent/popupWindow.hpp"

class PageChange : public juce::Component, public juce::Button::Listener,public juce::TextEditor::Listener{
//页码切换的组件
private:
    std::array<yTextButton,5> buttons;
    //按钮点击切换
    yTextButton goButton{U("go")};
    yTextButton button1{U("1")};//第一页
    yTextButton button_1;//最后一页
    YTextEditor mTextEditor;
    yTextButton previousButton{U("<")};
    yTextButton nextButton{U(">")};
    std::array<YLabel, 2> ellipsisLabels{U("..."),U("...")};
    int nowPage{1};
    int numPages{1};
    std::array<juce::Rectangle<int>, 9> pageChangeRects;//直接显示数字和省略号的那些区域
public:
    void resized() override;
    void setNumPages();
    PageChange();
    ~PageChange();
    void buttonClicked (Button*) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void numPagesChange();//根据总页数放置按钮的位置
    void nowPageChange();//当前页面切换时调用
    void setSelectedButtonColor();//设置当前选中的按钮颜色

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
