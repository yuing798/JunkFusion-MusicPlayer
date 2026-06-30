#include "AllMusicPage.hpp"
#include "BinaryData.h"
#include "FontAbout/font.h"
#include "allMusicPage.hpp"
#include "buttons.hpp"
#include "databaseManage.hpp"
#include "fileMessage.hpp"
#include "fileUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherComponent.hpp"
#include "popupWindow.hpp"
#include "serial.hpp"
#include <memory>
#include <string>
#include <utility>

PageChange::PageChange(){
    for(auto& button : buttons){
        button.setSize(40, 40);
        button.addListener(this);
        addAndMakeVisible(button);
    }
    button1.setSize(40,40);
    button1.addListener(this);
    addAndMakeVisible(button1);

    button_1.setSize(40,40);
    button_1.addListener(this);
    addAndMakeVisible(button_1);

    goButton.addListener(this);
    goButton.setSize(40,40);

    previousButton.addListener(this);
    previousButton.setSize(40,40);
    
    nextButton.addListener(this);
    nextButton.setSize(40,40);
    
    addAndMakeVisible(goButton);
    addAndMakeVisible(previousButton);
    addAndMakeVisible(nextButton);

    mTextEditor.addListener(this);
    mTextEditor.setSize(170,40);
    addAndMakeVisible(mTextEditor);

    for(auto& ellipsisLabel :ellipsisLabels){
        addAndMakeVisible(ellipsisLabel);
        ellipsisLabel.setSize(40, 40);
    }
    setSize(360,80);

}
void PageChange::resized(){
    auto local{getLocalBounds()};
    local.removeFromTop(40);
    auto row2{local};
    doLayout();
    row2.removeFromLeft(80);
    mTextEditor.setBounds(row2.removeFromLeft(160).reduced(5));
    goButton.setBounds(row2.removeFromLeft(40).reduced(5));
}

void PageChange::buttonClicked (Button* button){
    if(&goButton == button){
        goAndEnterClick();
    }
    if(button == &button1 || button == &button_1 ){
        nowPage = button->getButtonText().getIntValue();
        nowPageChange();
    }
    for(auto& btn:buttons){
        if(button == & btn){
            nowPage = button->getButtonText().getIntValue();
            nowPageChange();
        }
    }
    if(button == &previousButton){
        nowPage = button->getButtonText().getIntValue()-1;
        nowPageChange();
    }
    if(button == &nextButton){
        nowPage = button->getButtonText().getIntValue()+1;
        nowPageChange();
    }

}

void PageChange::textEditorReturnKeyPressed(juce::TextEditor& editor){
    if(&editor == &mTextEditor){
        goAndEnterClick();
    }
}
void PageChange::goAndEnterClick(){
    auto targetPage{std::stoi(mTextEditor.getText().toStdString())};
    if(nowPage!=targetPage){
        nowPage = targetPage;
        //刷新页面
        //刷新按钮布局
        nowPageChange();
    }
}
void PageChange::numPagesChange()
{
    doLayout();
    nowPageChange();
}

void PageChange::nowPageChange()
{
    previousButton.setEnabled(nowPage > 1);
    nextButton.setEnabled(nowPage < numPages);

    // 用按钮文本与当前页码比较来高亮
    setPageButtonColor(button1);
    setPageButtonColor(button_1);
    for (auto& btn : buttons)
    {
        if (btn.isVisible())
            setPageButtonColor(btn);
    }
    if(mPageRefreshCallback) mPageRefreshCallback(nowPage);
}

void PageChange::doLayout()
{
    const int buttonW = 40;
    auto row1 = juce::Rectangle<int>(0, 0, getWidth(), 40);

    // ── 全部先隐藏 ──
    previousButton.setVisible(false);
    nextButton.setVisible(false);
    button1.setVisible(false);
    button_1.setVisible(false);
    for (auto& b : buttons)   b.setVisible(false);
    for (auto& e : ellipsisLabels) e.setVisible(false);

    if (numPages <= 0) return;

    // ── 收集分配按钮的 lambda ──
    int buttonPoolIdx = 0;
    auto getNextButton = [&](int pageNum) -> yTextButton&
    {
        if (pageNum == 1)            return button1;
        if (pageNum == numPages)     return button_1;
        auto& btn = buttons[buttonPoolIdx++];
        btn.setButtonText(juce::String(pageNum));
        return btn;
    };

    std::vector<juce::Component*> items;

    // Step 1: <
    items.push_back(&previousButton);

    // Step 2: 页码区域
    if (numPages <= 7)
    {
        for (int p = 1; p <= numPages; ++p)
            items.push_back(&getNextButton(p));
    }
    else
    {
        // > 7 页：中间 = [1] + 5槽 + [last]
        items.push_back(&button1);

        // 计算 5 槽中展示的页码区间
        int winStart, winEnd;
        if (nowPage <= 3)
        {
            // 靠近首页：多展示一些靠前的页码
            winStart = 2;
            winEnd   = 5;
        }
        else if (nowPage >= numPages - 2)
        {
            // 靠近末页：多展示一些靠后的页码
            winEnd   = numPages - 1;
            winStart = numPages - 4;
        }
        else
        {
            // 中间：紧凑展示 ±1
            winStart = nowPage - 1;
            winEnd   = nowPage + 1;
        }

        if (winStart > 2)
            items.push_back(&ellipsisLabels[0]);

        for (int p = winStart; p <= winEnd; ++p)
            items.push_back(&getNextButton(p));

        if (winEnd < numPages - 1)
            items.push_back(&ellipsisLabels[1]);

        items.push_back(&button_1);
    }

    // Step 3: >
    items.push_back(&nextButton);

    // ── 居中布局 ──
    int totalW = static_cast<int>(items.size()) * buttonW;
    int startX = row1.getCentreX() - totalW / 2;

    for (int i = 0; i < static_cast<int>(items.size()); ++i)
    {
        if (auto* comp = items[i])
        {
            comp->setBounds(juce::Rectangle<int>(
                startX + i * buttonW, row1.getY(), buttonW, buttonW).reduced(5));
            comp->setVisible(true);
        }
    }
}

void PageChange::setPageButtonColor(yTextButton& button)
{
    bool isCurrent = (button.getButtonText().getIntValue() == nowPage);

    if (isCurrent)
    {
        button.setColour(juce::TextButton::buttonColourId, ycolor.blackGrey);
        button.setColour(juce::TextButton::textColourOffId, ycolor.white);
    }
    else
    {
        button.setColour(juce::TextButton::buttonColourId, ycolor.shallowGrey);
        button.setColour(juce::TextButton::textColourOffId, ycolor.black);
    }
}

PageChange::~PageChange(){
    mTextEditor.removeListener(this);
    for(auto& button : buttons){
        button.removeListener(this);
    }
    goButton.removeListener(this);
    previousButton.removeListener(this);
    nextButton.removeListener(this);
}

AllMusicViewport::AllMusicViewport(AllMusicPage& a)
:mAllMusicPage(a){
    mPageChange.onPageChange([this](int value){
        refreshPage(value,mAllMusicPage.getSeqWays(),mAllMusicPage.getUpOrDown());
    });// 这里的代码在构造时不会执行，只是注册回调

    mPageChange.setNumPages(SongsManage::getInstance().getTotalSongCount()/numRows);
}
void AllMusicViewport::refreshPage(int page,int seqWayIndex, bool ascending){
    if(seqWayIndex == 0){
        //名称排序
        SongsManage::getInstance().getSongPageByName(page,numRows,ascending);
    }else if(seqWayIndex == 1){
        //添加时间排序
        SongsManage::getInstance().getSongsPageBySongId(page,numRows,ascending);
    }
    
}

void AllMusicViewport::resized(){
    for(auto& eachSongRow:eachSongRows){
        eachSongRow->setBounds(getLocalBounds().removeFromTop(70));
    }
    mPageChange.setTopLeftPosition((getWidth()-mPageChange.getWidth())/2.0f,0);

}
void LoadingGreyBlock::resized(){
    mLoadingAnimator.setBounds(getWidth()*0.4f,getHeight()*0.4,getWidth()*0.2f,getHeight()*0.2f);
}
void LoadingGreyBlock::paint(juce::Graphics& g){
    g.setColour(greyColor);
    g.fillAll();
}
void LoadingGreyBlock::start(){
    setVisible(true);
    mLoadingAnimator.start();
}
void LoadingGreyBlock::end(){
    mLoadingAnimator.end();
    setVisible(false);
}

AllMusicPage::AllMusicPage()
:mAllMusicViewport(*this){
    addAndMakeVisible(refreshButton);
    addAndMakeVisible(selectFileButton);
    addAndMakeVisible(allMusicLabel);
    addAndMakeVisible(mViewPort);
    addAndMakeVisible(numSongsLabel);
    addAndMakeVisible(mUpDownButton);

    selectFileButton.addListener(this);
    seqWays.addItem(U("歌曲名称排列"));
    seqWays.addItem(U("添加时间排列"));
    addAndMakeVisible(seqWays);

    int comboSelectedIndex{0};
    if(Serial::getInstance().getUIRoot().isValid()){
        comboSelectedIndex = Serial::getInstance().getUIRoot().getProperty(SERIAL_allMusicSeqWays,0);
    }
    seqWays.setSelectedItemIndex(comboSelectedIndex);

    seqWays.onItemSelected([this](int value){
        //更新排序方法
        //refreshPage(int page);//传参为当前的页码数
        //加入序列化
        Serial::getInstance().getUIRoot().setProperty(SERIAL_allMusicSeqWays, value, nullptr);
    });
    mAllMusicViewport.setSize(getWidth(),mAllMusicViewport.getViewportHeight());
}
void AllMusicPage::resized(){

    auto local = getLocalBounds();
    auto height = local.getHeight();
    auto viewportBounds{local.removeFromBottom(height * 0.8f)};
    mViewPort.setBounds(viewportBounds);
    mLoadingAnimator.setBounds(viewportBounds);//当在加载的时候直接把viewport给遮蔽掉
    auto row2 = local.removeFromBottom(height * 0.09f).reduced(5.0f);
    auto row1 = local.reduced(5);

    row2.removeFromLeft(20);
    selectFileButton.setBounds(row2.removeFromLeft(80).reduced(5,10));
    refreshButton.setBounds(row2.removeFromLeft(row2.getHeight()).reduced(10));
    row2.removeFromRight(40);
    mUpDownButton.setBounds(row2.removeFromRight(row2.getHeight()).reduced(10));
    seqWays.setBounds(local.removeFromRight(120).reduced(10));

    allMusicLabel.setBounds(row1.removeFromLeft(90));
    numSongsLabel.setBounds(row1.removeFromLeft(90));

}
void AllMusicPage::buttonClicked(juce::Button* button)
{
    if (button == &selectFileButton)
    {
        selectFileButton.setClickingTogglesState(false);//在推入数据库的时候先把按钮锁定
        getMultiMediaFileChoose(
            [](const juce::Array<juce::File>& selectedFiles)
            {
                for (auto& file : selectedFiles)
                {
                    auto eachSong{getStreamMetaData(file)};//FFmpeg提取原数据
                    if(!eachSong.filePath.empty()) SongsManage::getInstance().insertSong(eachSong);
                    //推入数据库
                }
            },
            this
        );
        
        //使用FFmpeg提取元数据
        //推入数据库

        //推入数据库后就可以清空了，等待下一次推入
        selectFileButton.setClickingTogglesState(true);
    }
}
// void AllMusicPage::paint(juce::Graphics& g){

// }