#include "BinaryData.h"
#include "FontAbout/font.h"
#include "buttons.hpp"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherComponent.hpp"
#include "popupWindow.hpp"
#include <memory>

class MyLikeButton : public juce::ToggleButton{
public:
    MyLikeButton(){
        setTooltip(U("添加到<我喜欢>中"));
    }
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class MultiStreamToolTip : public juce::TextButton{
public:
    MultiStreamToolTip(){
        setTooltip(U("该文件包含多路音频流（如多语言、多声道）。当前播放器将自动为您选择质量最佳的默认音轨。如需切换其他音轨，请使用专业音频工具（如 MKVToolNix）自行调整文件封装顺序"));
        setClickingTogglesState(false);
        setColour(juce::TextButton::textColourOffId, ycolor.white);
    }
    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override{
        g.setColour(ycolor.darkGrey);
        g.fillRoundedRectangle(getLocalBounds().toFloat(),3.0f);
        g.setColour(ycolor.white);
        g.setFont(juce::FontOptions().withHeight(16.0f).withStyle("Bold"));
        g.drawText(U("多流音频"),getLocalBounds(),juce::Justification::centred,false);
    }
};

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
    YLabel aiLabel;//ai识别出来的体裁和情绪
    MultiStreamToolTip mMultiStreamToolTip;
    YLabel mSongLength;
    bool isPlay{false};//当前这首歌是否正在播放，如果正在播放的话就不显示序号了

    playPauseButton mPlayPauseButton;//播放按钮
    std::unique_ptr<PopupWindowButton> mMoreInfoButton;
    MyLikeButton myLikeButton;
    littleLabel hadPlayedNumLabel;
public:

    EachSong(int songNo,SongInfo info);
    void resized() override;
    // void paint(juce::Graphics& g) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EachSong)
};
