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

    void doLayout();//设置每一个按钮或者省略号的布局
    void setPageButtonColor(yTextButton& button);//用来强调按下去的按钮的颜色
    std::function<void(int)> mPageRefreshCallback;//根据选中的页码数刷新页面

public:
    void onPageChange(std::function<void(int)> callback){mPageRefreshCallback = std::move(callback);};
    void resized() override;
    void setNumPages(int pagesNum){numPages = pagesNum;}//根据数据库里面的总歌曲数目设置总页码数
    PageChange();
    ~PageChange();
    void buttonClicked (Button*) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;//按下输入框回车键时的操作
    void numPagesChange();//总页码数切换时调整页码布局
    void nowPageChange();//当前页码切换时调整按钮布局
    void goAndEnterClick();//按下跳转按钮或者按下输入框的enter键
    int getNowPage() const {return nowPage;}

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageChange)
}; 

class SongSelectViewport : public juce::Component{
private:
    PageChange mPageChange;//页面切换组件
    const int numRows{15};//一页有多少首歌曲
    const int songRowHeight{70};//一首歌占用的高度
    std::array<std::unique_ptr<EachSong>,15> eachSongRows;

    juce::String sortMode{""};
    bool ascendingWay{true};//升序或者降序，true为升序

public:

    const juce::String nameSortId = U("name-sort");
    const juce::String addTimeSortId = U("add-time-sort");
    const juce::String playTimeSortId = U("play-time-sort");//排序方案

    SongSelectViewport();
    void resized() override;
    void refreshPage(int page,juce::String selectedSortMode, bool ascending);//根据当前页码数，排序方法，升序或者降序来刷新页面

    //返回整个viewPort的高度，这里mPageChange已经完成初始化了所以mPageChang.getHeight()能够正常返回
    int getViewportHeight(){return numRows*songRowHeight+mPageChange.getHeight();}
    
    int getNowPage(){return mPageChange.getNowPage();}
    juce::String getNowSortMode(){return sortMode;}
    bool getNowAscendingWay(){return ascendingWay;}
    void setSortMode(juce::String mode);
    void setAscendWay(bool upOrDown);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongSelectViewport)
};

class LoadingGreyBlock : public juce::Component{
//当在切换的时候，viewPort全部置为灰色不可点击，并显示一个很大的加载动画
private:
    const juce::Colour greyColor{ycolor.midGrey.withAlpha(0.3f)};
    loadingAnimator mLoadingAnimator;//导入文件,页面切换的时候的刷新动画
public:
    LoadingGreyBlock() = default;
    // ~LoadingGreyBlock();
    void resized() override;
    void paint(juce::Graphics& g) override;
    void start();
    void end();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoadingGreyBlock)
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

    juce::Viewport mViewPort;//里面放置15首歌曲和页面切换组件
    upDownButton mUpDownButton;//切换升降排序的按钮
    yTextButton playAll{U("播放全部")};
    loadingAnimator mLoadingAnimator;
    SongSelectViewport mSongSelectViewport;
    LoadingGreyBlock mLoadingGreyBlock;

public:

    AllMusicPage();
    void resized() override;
    void buttonClicked(juce::Button*) override;
    int getSeqWays(){return seqWays.getSelectedItemIndex();}//得到排序方法对应的索引
    int getUpOrDown(){return mUpDownButton.getToggleState();}//得到升降排序
    // void paint(juce::Graphics& g) override;
    void startLoading();
    void endLoading();
    void updateSortMode();
    // void refreshPageWithSameComfig();//使用相同的配置(相同的页码和排序方式来刷新页面)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicPage)
};
