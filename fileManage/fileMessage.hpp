#pragma once

// #include "FontAbout/font.h"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <vector>
#include <string>

struct SongInfo
{
    // ── 1. 文件信息 ──
    std::string filePath;               // 文件完整路径
    int64_t      fileSize    = 0;        // 文件大小（字节）
    std::string lastModifiedTime;       // 文件最后一次修改时间
    // std::string addTime;                // 添加到应用的时间
    double duration     = 0.0;          // 歌曲时长（秒）时长应该每条流都一样

    // ── 3. 标签信息 ──
    std::string title;           //名称，因为有的音乐文件有自带的标题(不是文件名!!!)
    std::string artist;                 // 艺术家名称
    std::string album;                  // 专辑
    std::string albumArtist;            // 专辑艺术家
    std::string genre;                  // 体裁
    int         trackNumber = -1;        // 轨道号
    int         discNumber  = 0;        // 碟片号
    int         year        = 0;        // 发行年份
    std::string composer;               // 作曲者
    //这些基础数据都是文件容器层面的
    
    std::string imageHash;             //图片所对应的哈希值索引

    // ── 2. FFmpeg 解码层信息 ──
    bool isMultiStreamFile{0};
    
    int64_t    bitRate      = 0;            // 比特率（kbps）
    int sampleRate{0};          // 采样率（Hz）ffmpeg只能读取整数采样率，
    // 实际上也基本都是整数采样率，processBlock中用double采样率是为了计算精度平衡
    int    numChannels  = 0;            // 通道数
    int    bitDepth     = 0;            // 位深
    std::string codecName;              // 编码器名称

    // ── 4. AI 分析信息 ──，ai分析是和具体音频流相关的，所以没有必要放在文件层
    bool isMusic{false};//检测这个流是不是音乐资源，没有的话ai分析个屁
    std::string aiGenre;                // AI 分析体裁
    int      bpm         {0};      // 节拍数
    std::string key;                    // 调性（如 C major, A minor）        
    bool        aiProcessed = false;    // 是否已经进行过 AI 处理

    //5.用户信息
    bool isMyLike{0};//是否添加到了我喜欢列表
    std::string comment;                // 备注(用户写进去的)
    int hadPlayedNum{0};//已经播放了多少次

    //6.排序字段
    int nameId{0};//优先级排序之后的位置，逻辑上是不会变的

    //将songInfo转化为var，才能推送给js端
    static juce::var toVar(const SongInfo& song){
        auto obj{new juce::DynamicObject()};

        // ── 1. 文件信息 ──
        obj->setProperty("filePath",         juce::String(song.filePath));
        obj->setProperty("fileSize",         song.fileSize);
        obj->setProperty("lastModifiedTime", juce::String(song.lastModifiedTime));
        obj->setProperty("duration",         song.duration);

        // ── 2. 标签信息 ──
        obj->setProperty("title",       juce::String(song.title));
        obj->setProperty("artist",      juce::String(song.artist));
        obj->setProperty("album",       juce::String(song.album));
        obj->setProperty("albumArtist", juce::String(song.albumArtist));
        obj->setProperty("genre",       juce::String(song.genre));
        obj->setProperty("trackNumber", song.trackNumber);
        obj->setProperty("discNumber",  song.discNumber);
        obj->setProperty("year",        song.year);
        obj->setProperty("composer",    juce::String(song.composer));
        obj->setProperty("imageHash",   juce::String(song.imageHash));

        // ── 3. FFmpeg 解码层 ──
        obj->setProperty("isMultiStreamFile", song.isMultiStreamFile);
        obj->setProperty("bitRate",           song.bitRate);
        obj->setProperty("sampleRate",        song.sampleRate);
        obj->setProperty("numChannels",       song.numChannels);
        obj->setProperty("bitDepth",          song.bitDepth);
        obj->setProperty("codecName",         juce::String(song.codecName));

        // ── 4. AI 分析 ──
        obj->setProperty("isMusic",     song.isMusic);
        obj->setProperty("aiGenre",     juce::String(song.aiGenre));
        obj->setProperty("bpm",         song.bpm);
        obj->setProperty("key",         juce::String(song.key));
        obj->setProperty("aiProcessed", song.aiProcessed);

        // ── 5. 用户信息 ──
        obj->setProperty("isMyLike",     song.isMyLike);
        obj->setProperty("comment",      juce::String(song.comment));
        obj->setProperty("hadPlayedNum", song.hadPlayedNum);

        // ── 6. 排序 ──
        obj->setProperty("nameId", song.nameId);

        return juce::var(obj);
    }//将songInfo转化为var，才能推送给js端

};
