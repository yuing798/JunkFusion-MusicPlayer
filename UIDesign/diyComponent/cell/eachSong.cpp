

#include "eachSong.hpp"
#include "BinaryData.h"
#include "FontAbout/font.h"
#include "buttons.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "popupWindow.hpp"
#include <memory>
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

    auto imageFile = imageDirId.getChildFile(longUTF8(info.imageHash,".jpg"));
    juce::Image songImage{};
    if(imageFile.existsAsFile()){
        songImage = juce::ImageCache::getFromFile(imageFile);
    }
    juce::String extraInfo{longUTF8(
        "文件路径：",info.filePath,
        "\n文件名称：",info.fileName,
        "\n文件大小：",info.fileSize,
        "\n最后修改时间：",info.lastModifiedTime,
        "\n上传时间：",info.addTime,
        "\n专辑艺术家：",info.albumArtist,
        "\n体裁：",info.genre,
        "\n轨道号：",info.trackNumber,
        "\n碟片号：",info.discNumber,
        "\n发行年份：",info.year,
        "\n作曲者：",info.composer
        // "\n：",,
    )};
    if(info.numAudioStreams == 1){
        //如果只有一条流的话直接把剩余的所有信息都写进去
        extraInfo += longUTF8(
            "\n比特率：",info.streams[0].bitRate,
            "\n通道数：",info.streams[0].numChannels,
            "\n位深：",info.streams[0].bitDepth,
            "\n解码器名称：",info.streams[0].codecName,
            "\nAI分析体裁:",info.streams[0].aiGenre,
            "\nAI分析情绪:",info.streams[0].aiMood,
            "\n是否为音乐资源:",info.streams[0].isMusic ? U("是") : U("否"),
            "\n是否已经进行过AI分析:",info.streams[0].aiProcessed ? U("是") : U("否"),
            "\nBPM:",info.streams[0].bpm,
            "\n调性:",info.streams[0].key,
            "\n采样率:",info.streams[0].sampleRate,
            "\n额外信息:",info.extraMetadata + info.streams[0].extraMetadata
        );
    }else if(info.numAudioStreams > 1){
        //如果有多条流只写文件层的额外信息，流层的额外信息有单独的一张卡片
        extraInfo += longUTF8(
            "\n额外信息:",info.extraMetadata
        );
    }

    mMoreInfoButton = std::make_unique<PopupWindowButton>(
        std::make_unique<svgButton>(
            U("更多信息"),
            juce::Drawable::createFromImageData(BinaryData::whatsMore_svg, BinaryData::whatsMore_svgSize)
        ),
        songTitle,
        std::make_unique<OtherSongInfoIntro>(
            songImage,
            longUTF8("")
        )
    );
    
}
void EachSong::resized(){

}
void EachSong::paint(juce::Graphics& g){
    
}