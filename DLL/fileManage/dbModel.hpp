#pragma once

#include "dllAndFlutterBridge.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct SongInfo {
    int64_t songId{0};

    double duration{0.0}; // 歌曲时长（秒）时长应该每条流都一样

    // ── 3. 标签信息 ──
    juce::String title; // 名称，因为有的音乐文件有自带的标题(不是文件名!!!)
    // 这个标题不需要std::optional保底，因为我在FFmpeg提取元数据函数中已经写好了如果没有title直接把文件stem名称赋值给title

    juce::StringArray artists;      // 艺术家名称
    juce::String album;             // 专辑
    juce::String albumArtist;       // 专辑艺术家
    juce::String genre;             // 体裁
    std::optional<int> trackNumber; // 轨道号（std::nullopt 表示不存在）
    std::optional<int> discNumber;  // 碟片号（std::nullopt 表示不存在）
    std::optional<int> year;        // 发行年份（std::nullopt 表示不存在）
    juce::String composer;          // 作曲者
    // 这些基础数据都是文件容器层面的

    // ── 2. FFmpeg 解码层信息 ──

    int64_t bitRate = 0; // 比特率（kbps）
    int sampleRate{0};   // 采样率（Hz）ffmpeg只能读取整数采样率，
    // 实际上也基本都是整数采样率，processBlock中用double采样率是为了计算精度平衡
    juce::String channelLayout; // 通道布局(通道布局是一定有的)
    int bitDepth = 0;           // 位深
    juce::String codecName;     // 编码器名称//因为编码器名称不一定有，但是编码器ID一定有

    // ── 4. AI 分析信息 ──，ai分析是和具体音频流相关的，所以没有必要放在文件层
    juce::String aiGenre;   // AI 分析体裁
    std::optional<int> bpm; // 节拍数（std::nullopt 表示未知）
    juce::String key;       // 调性（如 C major, A minor）

    // 5.用户信息
    bool isMyLike{0};     // 是否添加到了我喜欢列表
    juce::String comment; // 备注(用户写进去的)
    int playNum{0};       // 已经播放了多少次

    juce::String hash;

    // 将songInfo转化为var，才能推送给js端
    juce::var toJson() {
        juce::var obj{new juce::DynamicObject()};

        // ── 辅助：optional<string> → juce::var ──
        auto optStr = [](const juce::String& v) -> juce::var {
            return v.isNotEmpty() ? juce::var(v) : juce::var();
        };
        auto optInt = [](const std::optional<int>& v) -> juce::var {
            return v.has_value() ? juce::var(v.value()) : juce::var();
        };
        auto ptr{obj.getDynamicObject()};

        // ── 0. 主键 ──
        ptr->setProperty(B_songInfo::songId, songId);

        // ── 1. 文件信息 ──
        ptr->setProperty(B_songInfo::duration, duration);

        // ── 2. 标签信息 ──
        ptr->setProperty(B_songInfo::title, title);
        ptr->setProperty(B_songInfo::artist, artists);
        ptr->setProperty(B_songInfo::album, optStr(album));
        ptr->setProperty(B_songInfo::albumArtist, optStr(albumArtist));
        ptr->setProperty(B_songInfo::genre, optStr(genre));
        ptr->setProperty(B_songInfo::trackNumber, optInt(trackNumber));
        ptr->setProperty(B_songInfo::discNumber, optInt(discNumber));
        ptr->setProperty(B_songInfo::year, optInt(year));
        ptr->setProperty(B_songInfo::composer, optStr(composer));

        // ── 3. FFmpeg 解码层 ──
        ptr->setProperty(B_songInfo::bitRate, bitRate);
        ptr->setProperty(B_songInfo::sampleRate, sampleRate);
        ptr->setProperty(B_songInfo::channelLayout, channelLayout);
        ptr->setProperty(B_songInfo::bitDepth, bitDepth);
        ptr->setProperty(B_songInfo::codecName, optStr(codecName));

        // ── 4. AI 分析 ──
        ptr->setProperty(B_songInfo::aiGenre, optStr(aiGenre));
        ptr->setProperty(B_songInfo::bpm, optInt(bpm));
        ptr->setProperty(B_songInfo::key, optStr(key));

        // ── 5. 用户信息 ──
        ptr->setProperty(B_songInfo::isMyLike, isMyLike);
        ptr->setProperty(B_songInfo::comment, optStr(comment));
        ptr->setProperty(B_songInfo::playNum, playNum);

        ptr->setProperty(B_songInfo::hash, optStr(hash));

        return juce::var(obj);
        // 这里不使用delete的原因是juce::var是引用计数的，共享所有权了，会自动delete
    } // 将songInfo转化为var，才能推送给js端
};

struct InsertSongInfo {
    SongInfo info;
    std::string errorMsg;
};

// songs 表：存储所有歌曲信息（文件层信息 + FFmpeg 解码层信息 + AI 分析信息 + 用户信息）
inline const char* createSongsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS songs (
        songId             INTEGER PRIMARY KEY AUTOINCREMENT,
        filePath           TEXT    UNIQUE NOT NULL,
        fileSize           INTEGER NOT NULL,
        lastModifiedTime   TEXT    NOT NULL,
        duration           REAL,
        title              TEXT,
        artists             TEXT,
        album              TEXT,
        albumArtist        TEXT,
        genre              TEXT,
        trackNumber        INTEGER,
        discNumber         INTEGER,
        year               INTEGER,
        composer           TEXT,
        bitRate            INTEGER,
        bitDepth           INTEGER,
        sampleRate         INTEGER,
        channelLayoutMask INTEGER,
        numChannels INYEGER,
        codecName TEXT, 
        aiGenre            TEXT,
        bpm                INTEGER,
        key                TEXT,
        aiProcessed        INTEGER DEFAULT 0,
        isMyLike           INTEGER DEFAULT 0,
        comment            TEXT,
        playNum       INTEGER DEFAULT 0,
        hash TEXT,
        timeDomainSpec BLOB
    )
)";