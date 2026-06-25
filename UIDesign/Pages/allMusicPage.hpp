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

class OtherSongInfoIntro : public juce::Component{
private:
    static constexpr float kPopupWidth = 400.0f;
    static constexpr float kImageSize = 200.0f;
    static constexpr float kPadding = 10.0f;

    juce::Image mImage;
    juce::String mText;
    
    juce::Font mTextFont{juce::FontOptions().withHeight(16.0f)};
    juce::TextLayout mTextLayout;
    float mTextHeight{0.0f};

public:
    float realHeight{0.0f};//如果信息太少会有一个最小高度
    OtherSongInfoIntro(juce::Image image, juce::String text);
    // ~OtherSongInfoIntro();
    // void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OtherSongInfoIntro)
};

class EachSong : public juce::Component{
private:

    littleLabel No_;//序号
    YLabel mName;
    littleLabel mArtist;
    YLabel mAlbum;

    PlayStopButton mPlayAndStopButton;//播放按钮
    // std::unique_ptr<SongIntroduce> mOtherInfo;//储存歌曲的额外信息

public:

    EachSong(int songNo,SongInfo info);
    void resized() override;
    void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EachSong)
};

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
