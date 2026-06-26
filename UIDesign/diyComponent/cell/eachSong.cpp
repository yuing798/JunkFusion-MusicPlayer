

#include "eachSong.hpp"
#include "juce_graphics/juce_graphics.h"
OtherSongInfoIntro::OtherSongInfoIntro(juce::Image image, juce::String text)
:mImage(image),mText(text){
    // 计算文本区域高度
    juce::AttributedString attributedText{mText};
    attributedText.setFont(mTextFont);
    attributedText.setWordWrap(juce::AttributedString::WordWrap::byWord);

    float textMaxWidth = kPopupWidth - 2.0f * kPadding;
    mTextLayout.createLayout(attributedText, textMaxWidth);
    mTextHeight = mTextLayout.getHeight();

    float imageSize{kImageSize};
    if(image.isNull()){
        imageSize = 0;
    }//如果没有放置图片则删掉图片的占位区域
    
    realHeight = juce::jmax(350.0f,kPadding + imageSize + kPadding + mTextHeight + kPadding);
    setSize(kPopupWidth, realHeight);
}

void OtherSongInfoIntro::paint(juce::Graphics& g){

    float currentY{0.0f};
    // 图片 — 若有图则 200×200 居中，无图则不占空间
    if (mImage.isValid())
    {
        float imageX = (kPopupWidth - kImageSize) / 2.0f;
        juce::Rectangle<float> imageBounds(imageX, currentY, kImageSize, kImageSize);
        g.drawImage(mImage, imageBounds, juce::RectanglePlacement::centred);
        currentY += kImageSize + kPadding;
    }

    // 文本 — 左对齐
    {
        juce::Rectangle<float> textBounds(kPadding, currentY,
                                           kPopupWidth - 2.0f * kPadding, mTextHeight);
        mTextLayout.draw(g, textBounds);
    }
}


EachSong::EachSong(int songNo,SongInfo info)

{
    addAndMakeVisible(mPlayPauseButton);

    No_.setText(juce::String(songNo), juce::dontSendNotification);
    addAndMakeVisible(No_);
    
    juce::String songTitle{""};//歌曲名称
    if(!info.title.empty()){
        songTitle = juce::String(info.title);
    }else{
        songTitle = juce::File(info.filePath).getFileNameWithoutExtension();
    }//如果文件没有title，直接使用文件的stem名称
    mName.setText(songTitle, juce::dontSendNotification);
    addAndMakeVisible(mName);

    juce::String artistName{U("未知")};
    if(!info.artist.empty()){
        artistName = juce::String(info.artist);
    }
    mArtist.setText(artistName, dontSendNotification);
    addAndMakeVisible(mArtist);

    juce::String albumName{U("未知")};
    if(!info.album.empty()){
        albumName = juce::String(info.album);
    }
    mAlbum.setText(albumName, dontSendNotification);
    addAndMakeVisible(mAlbum);
    
}
void EachSong::resized(){

}
void EachSong::paint(juce::Graphics& g){
    
}