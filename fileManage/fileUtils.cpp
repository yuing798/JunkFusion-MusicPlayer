#include "fileUtils.hpp"
#include "FontAbout/font.h"
#include "UISet.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <spdlog/spdlog.h>
#include <cstddef>
#include <chrono>
#include <memory>
#include <set>
#include <thread>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
}



void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)> onFileSelected,
                          juce::Component* parentComponent)
{

    // 1. 构建过滤器字符串（用分号分隔）
    juce::String filters = "*";

    // 2. 创建 FileChooser 对象（使用 shared_ptr 管理生命周期）
    auto chooser = std::make_shared<juce::FileChooser>(
        "请选择多媒体文件（音频或视频）",                // 对话框标题
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), // 初始目录
        filters,                                         // 过滤器字符串
        true,                                            // 使用原生对话框（外观更好）
        false,                                           // 不将包视为目录
        parentComponent                                  // 父组件（实现模态）
    );

    // 3. 异步启动对话框
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems, // 支持多选
        [chooser, onFileSelected](const juce::FileChooser&)//回调函数的传参只有在回调实际触发时才能知道
            
            // std::function：这是一个通用的函数包装器，意味着你可以传入任何可调用的对象：
            // 普通函数、函数指针、Lambda 表达式、std::bind 生成的对象等。

            // void：这个回调函数没有返回值。

            // const FileChooser &：回调被触发时，系统会把启动这次操作的 FileChooser 对象的常量引用传进来。
            
            {
                // 获取选中的所有文件
                juce::Array<juce::File> selected = chooser->getResults();

                // 调用回调，传递文件列表
                if (onFileSelected){//这个地方是在检查 std::function 这个“对象”是否为空
                    onFileSelected(selected);
                }
            }
    );
}

namespace
{
    // 安全字符串转 int，失败返回 0；处理 "3/12" 格式的轨道号
    int safeToInt(const char* str)
    {
        try { return std::stoi(str); }
        catch (...) { return 0; }
    }
}

SongInfo getMetaData(std::filesystem::path& filePath)
{
    SongInfo info{};  // 值初始化：数值类型为 0，std::string 为空

    // ═══════════════════════════════════════════════════════════════
    // 1. 文件信息（直接文件系统操作）
    // ═══════════════════════════════════════════════════════════════
    info.filePath = filePath.string();
    info.fileName = filePath.filename().string();

    juce::File juceFile(filePath.string());
    if (juceFile.existsAsFile())
    {
        info.fileSize = static_cast<size_t>(juceFile.getSize());
        info.lastModifiedTime = juceFile.getLastModificationTime()
                                    .toString(true, true, false, false)
                                    .toStdString();
    }

    info.addTime = juce::Time::getCurrentTime()
                       .toString(true, true, false, false)
                       .toStdString();

    // ═══════════════════════════════════════════════════════════════
    // 2. FFmpeg 打开文件
    // ═══════════════════════════════════════════════════════════════
    AVFormatContext* pFormatCtx = nullptr;

    if (avformat_open_input(&pFormatCtx,
                            filePath.string().c_str(),
                            nullptr, nullptr) != 0)
    {
        //非多媒体文件也会返回AVERROR
        //SPDLOG:记录多媒体文件无法打开文件或者打开的是非多媒体文件
        return {};  // 无法打开直接返回空
    }

    if (avformat_find_stream_info(pFormatCtx, nullptr) < 0)
    {
        //SPDLOG:记录无法找到流信息
        avformat_close_input(&pFormatCtx);
        return {};
    }

    // ═══════════════════════════════════════════════════════════════
    // 2. 音频流解码层信息
    // ═══════════════════════════════════════════════════════════════
    int audioStreamIndex = av_find_best_stream(pFormatCtx,
                                               AVMEDIA_TYPE_AUDIO,
                                               -1, -1, nullptr, 0);

    if (audioStreamIndex >= 0)
    {
        AVStream*           pAudioStream = pFormatCtx->streams[audioStreamIndex];
        AVCodecParameters*  pPar         = pAudioStream->codecpar;

        // 时长（秒）
        if (pFormatCtx->duration != AV_NOPTS_VALUE)
        {
            info.duration = static_cast<double>(pFormatCtx->duration)
                            / AV_TIME_BASE;
        }
        else if (pAudioStream->duration != AV_NOPTS_VALUE)
        {
            info.duration = static_cast<double>(pAudioStream->duration)
                            * pAudioStream->time_base.num
                            / pAudioStream->time_base.den;
        }

        // 比特率（kbps）——先取编码器报告值，缺失时用文件大小估算
        info.bitRate = pPar->bit_rate / 1000;
        if (info.bitRate <= 0 && info.duration > 0.0 && info.fileSize > 0)
        {
            info.bitRate = static_cast<int>(
                info.fileSize * 8.0 / info.duration / 1000.0);
        }

        // 采样率（Hz）
        info.sampleRate = static_cast<double>(pPar->sample_rate);

        // 通道数
        info.numChannels = pPar->ch_layout.nb_channels;

        // 位深 —— 仅 PCM 编码有意义，压缩编码 bits_per_coded_sample 为其解码位深
        int bytesPerSample = av_get_bytes_per_sample(
            static_cast<AVSampleFormat>(pPar->format));
        if (bytesPerSample > 0)
        {
            info.bitDepth = bytesPerSample * 8;
        }
        else
        {
            info.bitDepth = pPar->bits_per_coded_sample;
        }

        // 编码器名称
        const AVCodec* pCodec = avcodec_find_decoder(pPar->codec_id);
        if (pCodec != nullptr)
        {
            info.codecName = pCodec->long_name
                           ? pCodec->long_name
                           : pCodec->name;
        }
        else
        {
            info.codecName = avcodec_get_name(pPar->codec_id);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 3. 标签信息（读取容器级元数据）
    // ═══════════════════════════════════════════════════════════════
    AVDictionary*   pTags = pFormatCtx->metadata;
    AVDictionaryEntry* pEntry = nullptr;

    if ((pEntry = av_dict_get(pTags, "artist",       nullptr, 0)))
        info.artist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "album",        nullptr, 0)))
        info.album = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "album_artist", nullptr, 0)))
        info.albumArtist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "genre",        nullptr, 0)))
        info.genre = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "track",        nullptr, 0)))
        info.trackNumber = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "disc",         nullptr, 0)))
        info.discNumber = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "date",         nullptr, 0)))
        info.year = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "composer",     nullptr, 0)))
        info.composer = pEntry->value;

    // ── 清理 ──
    avformat_close_input(&pFormatCtx);

    return info;
}

void getMultiMediaFileDir(){
    
}