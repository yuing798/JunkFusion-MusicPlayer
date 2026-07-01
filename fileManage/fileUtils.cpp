#include "fileUtils.hpp"
#include "FontAbout/font.h"
#include "constants.h"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherUtils.hpp"
#include <spdlog/spdlog.h>
#include <cstddef>
#include <chrono>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include "sha1.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
}
void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)> onFileSelected,juce::Component* parentComponent)
{
    // 1. 构建过滤器字符串（用分号分隔）
    juce::String filters = "*";

    // 2. 创建 FileChooser 对象（使用 shared_ptr 管理生命周期）
    auto chooser = std::make_shared<juce::FileChooser>(
        U("请选择多媒体文件（音频或视频）"),                // 对话框标题
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
            {
                // std::function：这是一个通用的函数包装器，意味着你可以传入任何可调用的对象：
                // 普通函数、函数指针、Lambda 表达式、std::bind 生成的对象等。

                // void：这个回调函数没有返回值。

                // const FileChooser &：回调被触发时，系统会把启动这次操作的 FileChooser 对象的常量引用传进来。
                // 获取选中的所有文件
                juce::Array<juce::File> selected = chooser->getResults();

                // 调用回调，传递文件列表
                if (onFileSelected){//这个地方是在检查 std::function 这个“对象”是否为空
                    onFileSelected(selected);
                }
            }
    );
}
SongInfo getStreamMetaData(const juce::File& file){

    auto safeToInt = [](const char* str) -> int{
        try { return std::stoi(str); }
        catch (...) { return 0; }
    };

    SongInfo info{};
    info.filePath = file.getFullPathName().toStdString();
    info.fileName = file.getFileName().toStdString();
    info.fileSize = file.getSize();
    info.lastModifiedTime = file.getLastModificationTime().toString(true, true).toStdString();

    auto logger{spdlog::get(LogSchedulerID)};

    //ffmpeg解码层信息
    int result{0};//解码层结果，一般成功返回零
    AVFormatContext* inputContext{nullptr};
    result = avformat_open_input(&inputContext, info.filePath.c_str(), nullptr, nullptr);
    if(result!=0){
        //非多媒体文件也会返回AVERROR
        //SPDLOG:记录多媒体文件无法打开文件或者打开的是非多媒体文件
        if(logger) logger->warn("多媒体文件无法打开或者打开的是非多媒体文件:",ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return {};  // 无法打开直接返回空
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if(result<0){
        //SPDLOG:无法找到流信息
        if(logger) logger->error("无法找到该文件的流信息:",ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return {};
    }
    // av_find_best_stream(AVFormatContext *ic, enum AVMediaType type, int wanted_stream_nb, int related_stream, const struct AVCodec **decoder_ret, int flags)

    // 时长（秒）
    if (inputContext->duration != AV_NOPTS_VALUE)
    {
        info.duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
    }

    //这里进行封面提取
    AVPacket coverPacket;
    coverPacket.data = nullptr;
    coverPacket.size = 0;
    SHA1 sha1;
    for(size_t i=0; i<inputContext->nb_streams; i++){
        auto* stream{inputContext->streams[i]};
        auto type{stream->codecpar->codec_type};
        if(type == AVMEDIA_TYPE_ATTACHMENT || (type == AVMEDIA_TYPE_VIDEO && (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))){
            coverPacket = stream->attached_pic;
            break;
        }

    }
    if(coverPacket.data && coverPacket.size > 0){
        do{
            std::string hashHex = sha1(coverPacket.data,coverPacket.size);
            std::string fileName{hashHex + ".jpg"};
            juce::File filePath{imageDirId.getChildFile(fileName)};
            info.imageHash = hashHex;
            if(filePath.existsAsFile()) break;//如果这个文件已经存在，直接退出，避免保存两个相同图片
            juce::FileOutputStream outputStream(filePath);
            if(outputStream.openedOk()){
                outputStream.write(coverPacket.data, coverPacket.size);
                outputStream.flush();
            }

        }while(0);
    }
    int streamCount{0};
    for(size_t i = 0; i < inputContext->nb_streams; i++){
        if(inputContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            streamCount++;
            if(streamCount > 1){
                info.isMultiStreamFile = 1;
                break;
            }
        }
    }

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) >= 0};

    AVStream*           pAudioStream = inputContext->streams[currentIndex];
    AVCodecParameters*  decoderPar      = pAudioStream->codecpar;
    info.codecName = avcodec_get_name(decoderPar->codec_id);
    
    // 比特率（kbps）——先取编码器报告值，缺失时用文件大小估算
    info.bitRate = decoderPar->bit_rate / 1000;
    if (info.bitRate <= 0 && info.duration > 0.0 && info.fileSize > 0)
    {
        info.bitRate = static_cast<int>(
            info.fileSize * 8.0 / info.duration / 1000.0);
    }

    // 采样率（Hz）
    info.sampleRate = decoderPar->sample_rate;

    // 通道数
    info.numChannels = decoderPar->ch_layout.nb_channels;

    // 位深 —— 仅 PCM 编码有意义，压缩编码 bits_per_coded_sample 为其解码位深
    int bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(decoderPar->format));
    if (bytesPerSample > 0)
    {
        info.bitDepth = bytesPerSample * 8;
    }
    else
    {
        info.bitDepth = decoderPar->bits_per_coded_sample;
    }
    //提取文件层面的标签数据
    AVDictionary*   pTags = inputContext->metadata;
    AVDictionaryEntry* pEntry = nullptr;

    if ((pEntry = av_dict_get(pTags, "title",       nullptr, 0))){
        if(pEntry->value){
            info.title = pEntry->value;
        }else{
            info.title = juce::File(info.filePath).getFileNameWithoutExtension().toStdString();
        }
    }
        
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

    avformat_close_input(&inputContext);

    return info;
}