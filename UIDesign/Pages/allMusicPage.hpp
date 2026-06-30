#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "./diyComponent/otherComponent.hpp"
#include "./diyComponent/YComboBox.hpp"
#include "databaseManage.hpp"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include "./diyComponent/buttons.hpp"
#include "./diyComponent/popupWindow.hpp"
#include "./diyComponent/cell/eachSong.hpp"

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

    void doLayout();
    void setPageButtonColor(yTextButton& button, int pageNum);
    std::function<void(int)> mPageRefreshCallback;//根据选中的页码数刷新页面

public:
    void onPageChange(std::function<void(int)> callback){mPageRefreshCallback = std::move(callback);};
    void resized() override;
    void setNumPages();
    PageChange();
    ~PageChange();
    void buttonClicked (Button*) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void numPagesChange();
    void nowPageChange();
    void goAndEnterClick();//按下跳转按钮或者按下输入框的enter键

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageChange)
}; 

class AllMusicPage;

class AllMusicViewport : public juce::Component{
private:
    PageChange mPageChange;
    static const int numRows{20};
    std::array<std::unique_ptr<EachSong>,numRows> eachSongRows;

    AllMusicPage& mAllMusicPage;
public:
    explicit AllMusicViewport(AllMusicPage& a);
    void refreshPage(int page,int seqWayIndex, bool ascending);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicViewport)
};

class AllMusicPage : public juce::Component,
                     public juce::Button::Listener
{
private:

    
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
    AllMusicViewport mAllMusicViewport;

public:

    AllMusicPage();
    void resized() override;
    void buttonClicked(juce::Button*) override;
    int getSeqWays(){return seqWays.getSelectedItemIndex();}//得到排序方法对应的索引
    int getUpOrDown(){return mUpDownButton.getToggleState();}//得到升降排序
    // void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicPage)
};
