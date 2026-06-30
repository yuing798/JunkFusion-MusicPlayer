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
#include <utility>
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
    std::function<void(void)> mPageRefreshCallback;//根据选中的页码数刷新页面
    void setNumPages(int pagesNum);//根据数据库里面的总歌曲数目设置总页码数
    void numPagesLayoutChange();//总页码数切换时调整页码布局

public:
    void onPageChange(std::function<void(void)> callback){mPageRefreshCallback = std::move(callback);};
    void resized() override;
    
    PageChange();
    ~PageChange();//移除所有的监听
    void buttonClicked (Button*) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;//按下输入框回车键时的操作
    
    void nowPageChange();//当前页码切换时调整按钮布局
    void goAndEnterClick();//按下跳转按钮或者按下输入框的enter键
    int getNowPage() const {return nowPage;}
    void setNumpagesAndChangeLayout(int);//设置总页面数一定是和改变按钮区域的布局一定是同步发生的,传参为一页中有几首歌曲

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageChange)
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

class SongSelectViewport : public juce::Component{
private:
    PageChange mPageChange;//页面切换组件
    const int numRows{15};//一页有多少首歌曲
    const int songRowHeight{70};//一首歌占用的高度
    std::array<std::unique_ptr<EachSong>,15> eachSongRows;

    juce::String sortMode{""};
    bool ascendingWay{true};//升序或者降序，true为升序
    LoadingGreyBlock mLoadingGreyBlock;

public:
    PageChange& getMPageChange(){return mPageChange;}

    const juce::String nameSortId = U("name-sort");
    const juce::String addTimeSortId = U("add-time-sort");
    const juce::String playTimeSortId = U("play-time-sort");//排序方案

    SongSelectViewport();
    void resized() override;
    void refreshPage();//根据当前页码数，排序方法，升序或者降序来刷新页面

    //返回整个viewPort的高度，这里mPageChange已经完成初始化了所以mPageChang.getHeight()能够正常返回
    int getViewportHeight(){return numRows*songRowHeight+mPageChange.getHeight();}
    
    juce::String getNowSortMode(){return sortMode;}
    bool getNowAscendingWay(){return ascendingWay;}
    void setSortMode(juce::String mode){sortMode = mode;}
    void setAscendWay(bool upOrDown){ascendingWay = upOrDown;}
    void addLoadingGreyBlock();//LoadingGreyBlock应该为父类的子组件，和SongSelectViewport平级，所以需要一个单独的函数来让父类看到
    void startLoading();//开始加载动画
    void endLoading();//结束加载动画
    void setNumPages(int nb);//设置总页数，这里是要交给导入歌曲按钮来使用
    int getNumSongsEachPage(){return numRows;}//返回一个页码中最多有几条歌曲
    void init();//因为viewport的页面初始化需要外界的构造函数先执行，所以此处使用延迟执行的方法
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SongSelectViewport)
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

public:

    AllMusicPage();
    void resized() override;
    void buttonClicked(juce::Button*) override;
    // void paint(juce::Graphics& g) override;
    void updateSortMode();//更新排序模式
    void updateAscending();//更新升序或降序模式

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AllMusicPage)
};
