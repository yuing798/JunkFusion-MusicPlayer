#pragma once

#include "BinaryData.h"
#include "FontAbout/font.h"
#include "./diyComponent/otherComponent.hpp"
#include "./diyComponent/YComboBox.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <filesystem>
#include <vector>
#include "./diyComponent/buttons.hpp"
#include "songIntroduce.hpp"


class EachSong : public juce::Component{
private:

    littleLabel No_;//序号
    YLabel name;
    littleLabel composer;
    YLabel album;
    YLabel style;

    PlayStopButton mPlayAndStopButton;
    WhatsMoreToolTip mWhatsMore;//悬停显示：播放次数、时长、比特率

public:

    EachSong(int songNo,
        const juce::String& songName,
        const juce::String& songComposer,
        const juce::String& songAlbum,
        const juce::String& songStyle,
        const juce::String& playNums,
        const juce::String& songLength,
        const juce::String& songBitRate
    );
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
    // SongIntroduce mSongIntroduce{juce::Image{}, U("1"), U("歌曲信息")};

public:

    allMusicPage();
    void resized() override;
    void buttonClicked(juce::Button*) override;
    // void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(allMusicPage)
};
