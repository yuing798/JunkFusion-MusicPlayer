#pragma once
#include <cstddef>
#include <juce_gui_basics/juce_gui_basics.h>
#include <filesystem>
#include <string>

//异步打开文件选择框，支持多选，回调返回选中的文件数组
void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)>,juce::Component* parentComponent = nullptr);

//获取某个文件夹下面的所有合适的多媒体文件
void getMultiMediaFileDir();

//使用FFmpeg提取元数据

struct SongInfo
{
    // ── 1. 文件信息 ──
    std::string filePath;               // 文件完整路径
    std::string fileName;               // 文件名称（含扩展名）
    size_t      fileSize    = 0;        // 文件大小（字节）
    std::string lastModifiedTime;       // 文件最后一次修改时间
    std::string addTime;                // 添加到应用的时间

    // ── 2. FFmpeg 解码层信息 ──
    double duration     = 0.0;          // 歌曲时长（秒）
    int    bitRate      = 0;            // 比特率（kbps）
    double sampleRate   = 0.0;          // 采样率（Hz）
    int    numChannels  = 0;            // 通道数
    int    bitDepth     = 0;            // 位深
    std::string codecName;              // 编码器名称

    // ── 3. 标签信息 ──
    std::string title;           //名称，因为有的音乐文件有自带的标题(不是文件名!!!)
    std::string artist;                 // 艺术家名称
    std::string album;                  // 专辑
    std::string albumArtist;            // 专辑艺术家
    std::string genre;                  // 体裁
    int         trackNumber = 0;        // 轨道号
    int         discNumber  = 0;        // 碟片号
    int         year        = 0;        // 发行年份
    std::string composer;               // 作曲者

    // ── 4. AI 分析信息 ──
    std::string aiGenre;                // AI 分析体裁
    std::string aiMood;                 // AI 分析情绪
    double      bpm         = 0.0;      // 节拍数
    std::string key;                    // 调性（如 C major, A minor）
    bool        aiProcessed = false;    // 是否已经进行过 AI 处理

    // ── 5. 其他信息 ──
    std::string comment;                // 备注/杂项
};

SongInfo getMetaData(std::filesystem::path&);
