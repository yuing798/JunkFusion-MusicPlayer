#include "./ffmpegDecoder.hpp"
#include "constants.h"
#include "dbManager.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Statement.h>
#include <cstdint>

#include <libavcodec/packet.h>
#include <libavutil/samplefmt.h>
#include <spdlog/spdlog.h>

extern "C" {
#include "libavutil/avutil.h"
#include <libavcodec/avcodec.h> //负责编解码
#include <libavcodec/codec.h>
#include <libavcodec/codec_id.h>
#include <libavformat/avformat.h> // 负责解封装
#include <libavutil/channel_layout.h>
#include <libavutil/log.h> //负责日志信息
#include <libswresample/swresample.h>
}

FFmpegDecoder::FFmpegDecoder(AudioRingBuffer& b) : juce::Thread("Decoder"), ringBuffer(b) {
    setPriority(juce::Thread::Priority::highest); // 最高优先级
}

FFmpegDecoder::~FFmpegDecoder() {
    // 给予线程 2000 毫秒的时间完成当前的 while 循环并退出。
    // 如果 2000ms 后仍未退出，系统会强制终止
    stopThread(2000);
}

void FFmpegDecoder::prepareToPlay(juce::AudioChannelSet juceLayout, double s) {

    sampleRate = s;
    auto mapJuceSpeakerToFFmpegMask = [](juce::AudioChannelSet::ChannelType juceType) -> uint64_t {
        switch (juceType) {
        case juce::AudioChannelSet::left: return AV_CH_FRONT_LEFT;                   // 0x00000001
        case juce::AudioChannelSet::right: return AV_CH_FRONT_RIGHT;                 // 0x00000002
        case juce::AudioChannelSet::centre: return AV_CH_FRONT_CENTER;               // 0x00000004
        case juce::AudioChannelSet::LFE: return AV_CH_LOW_FREQUENCY;                 // 0x00000008
        case juce::AudioChannelSet::leftSurround: return AV_CH_BACK_LEFT;            // 0x00000010
        case juce::AudioChannelSet::rightSurround: return AV_CH_BACK_RIGHT;          // 0x00000020
        case juce::AudioChannelSet::leftCentre: return AV_CH_FRONT_LEFT_OF_CENTER;   // 0x00000040
        case juce::AudioChannelSet::rightCentre: return AV_CH_FRONT_RIGHT_OF_CENTER; // 0x00000080
        case juce::AudioChannelSet::centreSurround: return AV_CH_BACK_CENTER;        // 0x00000100
        case juce::AudioChannelSet::leftSurroundSide: return AV_CH_SIDE_LEFT;        // 0x00000200
        case juce::AudioChannelSet::rightSurroundSide: return AV_CH_SIDE_RIGHT;      // 0x00000400
        case juce::AudioChannelSet::topMiddle: return AV_CH_TOP_CENTER;              // 0x00000800
        case juce::AudioChannelSet::topFrontLeft: return AV_CH_TOP_FRONT_LEFT;       // 0x00001000
        case juce::AudioChannelSet::topFrontCentre: return AV_CH_TOP_FRONT_CENTER;   // 0x00002000
        case juce::AudioChannelSet::topFrontRight: return AV_CH_TOP_FRONT_RIGHT;     // 0x00004000
        case juce::AudioChannelSet::topRearLeft: return AV_CH_TOP_BACK_LEFT;         // 0x00008000
        case juce::AudioChannelSet::topRearCentre: return AV_CH_TOP_BACK_CENTER;     // 0x00010000
        case juce::AudioChannelSet::topRearRight: return AV_CH_TOP_BACK_RIGHT;       // 0x00020000
        case juce::AudioChannelSet::LFE2: return AV_CH_LOW_FREQUENCY_2; // 0x0000000800000000ULL
        case juce::AudioChannelSet::leftSurroundRear: return AV_CH_BACK_LEFT;
        case juce::AudioChannelSet::rightSurroundRear: return AV_CH_BACK_RIGHT;
        case juce::AudioChannelSet::wideLeft: return AV_CH_WIDE_LEFT;   // 0x0000000080000000ULL
        case juce::AudioChannelSet::wideRight: return AV_CH_WIDE_RIGHT; // 0x0000000100000000ULL
        case juce::AudioChannelSet::topSideLeft: return AV_CH_TOP_SIDE_LEFT;
        case juce::AudioChannelSet::topSideRight: return AV_CH_TOP_SIDE_RIGHT;

        default: return 0; // 无法识别的空间位置
        }
    };

    uint64_t outputChannelLayoutMask{0};

    for (auto& type : juceLayout.getChannelTypes()) {
        outputChannelLayoutMask |= mapJuceSpeakerToFFmpegMask(type);
    }

    if (outputChannelLayoutMask == 0) {
        av_channel_layout_default(&outputChannelLayout, juceLayout.getChannelTypes().size());
    } else {
        av_channel_layout_from_mask(&outputChannelLayout, outputChannelLayoutMask);
    }
}

void FFmpegDecoder::prepareToPlayNewSong(int64_t id) { currentSongId = id; }

void FFmpegDecoder::run() {

    av_log_set_level(AV_LOG_ERROR);

    AVFormatContext* inputContext;
    path = dbManager::getInstance().getSongsManager().getPathBySongId(currentSongId);
    if (path.empty()) {
        // 弹出错误弹窗
        auto log = spdlog::get(LogAudioID);
        log->error("获取播放信息失败");
    }
    int result = avformat_open_input(
        &inputContext,
        path.c_str(),
        NULL,
        NULL
    ); // 这个函数会同时进行内存分配
    if (result < 0) {
        auto log = spdlog::get(LogAudioID);
        log->error("打开多媒体文件失败:文件路径:{}:错误原因:{}", path, ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return;
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if (result < 0) {
        auto log = spdlog::get(LogAudioID);
        log->error(
            "获取流失败，通知用户检查原始文件:{},失败原因:{}",
            path,
            ffmpegErrorOutput(result)
        );
        avformat_close_input(&inputContext);
        return;
    }

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* decoderPar = inputContext->streams[currentIndex]->codecpar;
    auto codec = avcodec_find_decoder(decoderPar->codec_id);
    auto* decoderContext = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(decoderContext, decoderPar); // 连接参数和上下文
    avcodec_open2(decoderContext, codec, nullptr);             // 解码器初始化
    SwrContext* swrContext{nullptr};
    swr_alloc_set_opts2( // 下面初始化重采样器
        &swrContext, 
        &outputChannelLayout, 
        AV_SAMPLE_FMT_FLTP,//float 且平面结构 
        sampleRate, 
        &decoderPar->ch_layout,
        static_cast<AVSampleFormat>(decoderPar->format), 
        decoderPar->sample_rate, 
        0, 
        nullptr
    );
    swr_init(swrContext);

    auto* packet =
        av_packet_alloc(); // 这里分配的只是内存地址，当下一个包开始处理后，覆盖原本的信息
    auto* frame = av_frame_alloc();

    // 输出数组指针
    juce::AudioBuffer<float> buffer;
    uint8_t** outputDataArray;

    while (av_read_frame(inputContext, packet) == 0) {
        if (packet->stream_index != currentIndex) {
            av_packet_unref(packet);
            continue;
        }
        result = avcodec_send_packet(decoderContext, packet);
        av_packet_unref(packet);
        if (result != 0) continue;
        while (avcodec_receive_frame(decoderContext, frame) == 0) {
            auto delayNumSamples = swr_get_delay(swrContext, frame->sample_rate);
            int numOutputSamples = av_rescale_rnd(
                delayNumSamples + frame->nb_samples,
                sampleRate,
                decoderPar->sample_rate,
                AV_ROUND_UP
            );
            buffer.setSize(//只分配第一次内存，第二次自动跳过
                outputChannelLayout.nb_channels, 
                numOutputSamples, 
                false, 
                false, 
                true
            );//如果数组内的样本已经是numOutputSamples了就不会重新执行
            int outputLineSize{0};
            result = av_samples_alloc_array_and_samples(
                &outputDataArray, // 输出通道指针数组的起始地址
                &outputLineSize,  // 物理字节大小
                outputChannelLayout.nb_channels,
                numOutputSamples,
                AV_SAMPLE_FMT_FLTP,
                0
            ); // 第二个参数和第四个参数的区别：nb_samples 是逻辑样本数（比如 1024 个浮点样本）。
            // linesize 是物理字节大小（比如对于 1024 个浮点样本，linesize 通常是 1024 * 4 = 4096
            // 字节，但如果内存对齐强制要求 64 字节对齐，它可能是 4096 或 4096+）。

            if (result < 0) {
                av_frame_unref(frame);

                av_freep(&outputDataArray[0]);
                av_freep(&outputDataArray);
                continue;
            }

            result = swr_convert(
                swrContext,
                outputDataArray,  // 这里面装的就是重采样后的PCM数据
                numOutputSamples, // 输出缓冲区的最大容量(理论最大值)
                static_cast<uint8_t**>(frame->data),
                frame->nb_samples
            ); // 执行重采样，返回实际重采样完的样本点个数

            av_frame_unref(frame); // 这时候frame已经没有用了

            if (numOutputSamples <= 0) {
                // 没有数据，清理并继续
                av_freep(&outputDataArray[0]);
                av_freep(&outputDataArray);
                continue;
            }

            for (int ch = 0; ch < outputChannelLayout.nb_channels; ++ch) {
                float* dest = buffer.getWritePointer(ch);
                const float* src = reinterpret_cast<const float*>(outputDataArray[ch]);
                std::memcpy(dest, src, numOutputSamples * sizeof(float));
            }
        }
    }

    // 线程即将退出，这里可以放置 FFmpeg 上下文的安全销毁逻辑 (avcodec_free_context 等)
}