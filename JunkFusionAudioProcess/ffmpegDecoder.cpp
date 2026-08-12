#include "./ffmpegDecoder.hpp"
#include "constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Statement.h>
#include <cstdint>

#include <spdlog/spdlog.h>

extern "C" {
#include "libavutil/avutil.h"
#include <libavcodec/avcodec.h> //负责编解码
#include <libavcodec/codec.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h> // 负责解封装
#include <libavutil/channel_layout.h>
#include <libavutil/log.h> //负责日志信息
#include <libavutil/samplefmt.h>
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
    // path = dbManager::getInstance().getSongsManager().getPathBySongId(currentSongId);
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
    auto codec = avcodec_find_decoder(decoderPar->codec_id);   // 根据ID寻找解码器
    auto* decoderContext = avcodec_alloc_context3(codec);      // 分配解码器上下文空间
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

    auto* packet = av_packet_alloc();
    // 这里分配的只是内存地址，当下一个包开始处理后，覆盖原本的信息
    auto* frame = av_frame_alloc();

    // 输出数组指针
    juce::AudioBuffer<float> buffer;
    uint8_t** outputDataArray;

    auto swrAndPushIntoFifo = [&] {
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
            // 这个函数的目的是清空重置，而av_frame_free的作用是彻底删除
            // 前面那个可以理解做清空数组，后面那个意味着连数组的内存也一起销毁

            if (numOutputSamples <= 0) {
                // 没有数据，清理并继续
                av_freep(&outputDataArray[0]);
                av_freep(&outputDataArray);
                // outputDataArray[0] 指向的是真正的音频数据块，而
                // outputDataArray 本身是“存放这些指针的数组”（通常只有几十字节）
                // 所以需要先释放一级然后释放二级
                continue;
            }

            for (int ch = 0; ch < outputChannelLayout.nb_channels; ++ch) {
                float* dest = buffer.getWritePointer(ch);
                const float* src = reinterpret_cast<const float*>(outputDataArray[ch]);
                std::memcpy(dest, src, numOutputSamples * sizeof(float));
            }
            av_freep(&outputDataArray[0]);
            av_freep(&outputDataArray);

            ringBuffer.pushAudioData(buffer);
        }
    };

    while (av_read_frame(inputContext, packet) == 0) {
        if (packet->stream_index != currentIndex) {
            av_packet_unref(packet);
            continue;
        }
        // 在音频中一个packet包含着多个frame，而视频只有一个frame
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
            // 这个函数的目的是清空重置，而av_frame_free的作用是彻底删除
            // 前面那个可以理解做清空数组，后面那个意味着连数组的内存也一起销毁

            if (numOutputSamples <= 0) {
                // 没有数据，清理并继续
                av_freep(&outputDataArray[0]);
                av_freep(&outputDataArray);
                // outputDataArray[0] 指向的是真正的音频数据块，而
                // outputDataArray 本身是“存放这些指针的数组”（通常只有几十字节）
                // 所以需要先释放一级然后释放二级
                continue;
            }

            for (int ch = 0; ch < outputChannelLayout.nb_channels; ++ch) {
                float* dest = buffer.getWritePointer(ch);
                const float* src = reinterpret_cast<const float*>(outputDataArray[ch]);
                std::memcpy(dest, src, numOutputSamples * sizeof(float));
            }
            av_freep(&outputDataArray[0]);
            av_freep(&outputDataArray);

            ringBuffer.pushAudioData(buffer);
        }
    }

    // 下面执行flush操作
    // 让解码器把内部缓存的所有未输出的帧全部吐出来
    // 发送空包，触发解码器输出缓冲帧
    // 这一次flush 操作的数据来源不再是“文件”而是“解码器内部的缓存区”，所以不需要再次av_read_frame
    avcodec_send_packet(decoderContext, nullptr);
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
        // 这个函数的目的是清空重置，而av_frame_free的作用是彻底删除
        // 前面那个可以理解做清空数组，后面那个意味着连数组的内存也一起销毁

        if (numOutputSamples <= 0) {
            // 没有数据，清理并继续
            av_freep(&outputDataArray[0]);
            av_freep(&outputDataArray);
            // outputDataArray[0] 指向的是真正的音频数据块，而
            // outputDataArray 本身是“存放这些指针的数组”（通常只有几十字节）
            // 所以需要先释放一级然后释放二级
            continue;
        }

        for (int ch = 0; ch < outputChannelLayout.nb_channels; ++ch) {
            float* dest = buffer.getWritePointer(ch);
            const float* src = reinterpret_cast<const float*>(outputDataArray[ch]);
            std::memcpy(dest, src, numOutputSamples * sizeof(float));
        }
        av_freep(&outputDataArray[0]);
        av_freep(&outputDataArray);

        ringBuffer.pushAudioData(buffer);
    }

    // ==========================================
    // 第三阶段：冲刷重采样器（Flush Resampler）
    // ==========================================
    // 即便解码器已经没有帧了，重采样器内部因为滤波算法（如 sinc 插值）可能还存着几个样本。
    // 必须用 NULL 输入把残渣挤出来。

    int maxFlushSamples = swr_get_out_samples(swrContext, 0);
    // 参数 0 表示“不喂入任何新输入，只计算内部残余需要多少输出空间”

    if (maxFlushSamples > 0) {
        uint8_t** flushData = nullptr;
        int flushLinesize = 0;
        av_samples_alloc_array_and_samples(
            &flushData,
            &flushLinesize,
            outputChannelLayout.nb_channels,
            maxFlushSamples,
            AV_SAMPLE_FMT_FLTP,
            0
        );

        while (true) {
            int ret = swr_convert(swrContext, flushData, maxFlushSamples, nullptr, 0);
            if (ret <= 0) break;
            // 将 ret 个样本推入 FIFO
            for (int ch = 0; ch < outputChannelLayout.nb_channels; ++ch) {
                float* dest = buffer.getWritePointer(ch);
                const float* src = reinterpret_cast<const float*>(outputDataArray[ch]);
                std::memcpy(dest, src, maxFlushSamples * sizeof(float));
            }
            av_freep(&flushData[0]);
            av_freep(&flushData);

            ringBuffer.pushAudioData(buffer);
        }
        // 释放 Flush 临时缓冲区
        av_freep(&flushData[0]);
        av_freep(&flushData);
    }

    av_frame_free(&frame);
    av_packet_free(&packet);

    swr_free(&swrContext);
    avcodec_free_context(&decoderContext);
    avformat_close_input(&inputContext);
}