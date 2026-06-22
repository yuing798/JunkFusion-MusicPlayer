#include "fileUtils.hpp"
#include "FontAbout/font.h"
#include "UISet.h"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <libavcodec/codec.h>
#include <libavcodec/codec_id.h>
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



void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)> onFileSelected,juce::Component* parentComponent)
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

SongInfo getFileMetaData(const juce::File& name){
    SongInfo info{};
    info.filePath = name.getFullPathName().toStdString();
    info.fileName = name.getFileName().toStdString();
    info.fileSize = name.getSize();
    info.lastModifiedTime = name.getLastModificationTime().toString(true, true).toStdString();
    info.addTime = juce::Time::getCurrentTime().toString(true, true).toStdString();

    return info;
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
    info.addTime = juce::Time::getCurrentTime().toString(true, true).toStdString();

    //ffmpeg解码层信息
    int result{0};//解码层结果，一般成功返回零
    AVFormatContext* inputContext{nullptr};
    result = avformat_open_input(&inputContext, info.filePath.c_str(), nullptr, nullptr);
    if(result!=0){
        //非多媒体文件也会返回AVERROR
        //SPDLOG:记录多媒体文件无法打开文件或者打开的是非多媒体文件
        avformat_close_input(&inputContext);
        return {};  // 无法打开直接返回空
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if(result<0){
        //SPDLOG:无法找到流信息
        avformat_close_input(&inputContext);
        return {};
    }
    std::vector<int> audioStreamIndex;

    for(size_t j = 0; j < inputContext->nb_streams; j++){
        if(inputContext->streams[j]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            //验证编码器类型，编码器类型由流类型决定。
            audioStreamIndex.push_back(j);
        }
    }//因为一个文件中可能拥有多个音频流，比如一个视频，有英文流，法语流，中文流同时存在
    if(audioStreamIndex.empty()){
        //SPDLOG:打开的文件并没有音频流
        avformat_close_input(&inputContext);//如果这个文件没有音频流就释放资源
        return {};
    }
    info.numAudioStreams = audioStreamIndex.size();
    info.streams.resize(audioStreamIndex.size());

    int streamCount{0};//最终提取流个数计数器

    for(size_t i=0; i<inputContext->nb_streams; i++){//按每条流迭代

        auto currentIndex{audioStreamIndex[i]};

        AVStream*           pAudioStream = inputContext->streams[currentIndex];
        AVCodecParameters*  decoderPar      = pAudioStream->codecpar;
        auto* decoder = avcodec_find_decoder(decoderPar->codec_id);
        if(decoder == nullptr){
            //这一个流索引没有音频编码器，跳过就可以了，不需要报错
            continue;
        }
        info.streams[streamCount].streamCount = streamCount;
        streamCount++;
        info.streams[streamCount].codecName = avcodec_get_name(decoderPar->codec_id);
        
        // 时长（秒）
        if (inputContext->duration != AV_NOPTS_VALUE)
        {
            info.streams[streamCount].duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
        }
        else if (pAudioStream->duration != AV_NOPTS_VALUE)
        {
            info.streams[streamCount].duration = static_cast<double>(pAudioStream->duration)
                            * pAudioStream->time_base.num
                            / pAudioStream->time_base.den;
        }

        // 比特率（kbps）——先取编码器报告值，缺失时用文件大小估算
        info.streams[streamCount].bitRate = decoderPar->bit_rate / 1000;
        if (info.streams[streamCount].bitRate <= 0 && info.streams[streamCount].duration > 0.0 && info.fileSize > 0)
        {
            info.streams[streamCount].bitRate = static_cast<int>(
                info.fileSize * 8.0 / info.streams[streamCount].duration / 1000.0);
        }

        // 采样率（Hz）
        info.streams[streamCount].sampleRate = decoderPar->sample_rate;

        // 通道数
        info.streams[streamCount].numChannels = decoderPar->ch_layout.nb_channels;

        // 位深 —— 仅 PCM 编码有意义，压缩编码 bits_per_coded_sample 为其解码位深
        int bytesPerSample = av_get_bytes_per_sample(
            static_cast<AVSampleFormat>(decoderPar->format));
        if (bytesPerSample > 0)
        {
            info.streams[streamCount].bitDepth = bytesPerSample * 8;
        }
        else
        {
            info.streams[streamCount].bitDepth = decoderPar->bits_per_coded_sample;
        }

        //提取标签数据
        AVDictionary*   pTags = inputContext->metadata;
        AVDictionaryEntry* pEntry = nullptr;

        if ((pEntry = av_dict_get(pTags, "title",       nullptr, 0)))
            info.streams[streamCount].artist = pEntry->value;
        if ((pEntry = av_dict_get(pTags, "artist",       nullptr, 0)))
            info.streams[streamCount].artist = pEntry->value;
        if ((pEntry = av_dict_get(pTags, "album",        nullptr, 0)))
            info.streams[streamCount].album = pEntry->value;
        if ((pEntry = av_dict_get(pTags, "album_artist", nullptr, 0)))
            info.streams[streamCount].albumArtist = pEntry->value;
        if ((pEntry = av_dict_get(pTags, "genre",        nullptr, 0)))
            info.streams[streamCount].genre = pEntry->value;
        if ((pEntry = av_dict_get(pTags, "track",        nullptr, 0)))
            info.streams[streamCount].trackNumber = safeToInt(pEntry->value);
        if ((pEntry = av_dict_get(pTags, "disc",         nullptr, 0)))
            info.streams[streamCount].discNumber = safeToInt(pEntry->value);
        if ((pEntry = av_dict_get(pTags, "date",         nullptr, 0)))
            info.streams[streamCount].year = safeToInt(pEntry->value);
        if ((pEntry = av_dict_get(pTags, "composer",     nullptr, 0)))
            info.streams[streamCount].composer = pEntry->value;

    }
    avformat_close_input(&inputContext);

    return info;
}