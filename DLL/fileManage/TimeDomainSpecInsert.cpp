#include "./TimeDomainSpecInsert.hpp"
#include "constants.h"
#include "dllUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstring>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

TimeDomainSpecInsert::TimeDomainSpecInsert(SQLite::Database& db)
    : juce::Thread("TimeDomainSpecInsert"), mDb(db) {}

void TimeDomainSpecInsert::setTask(std::string file) {
    std::lock_guard<std::mutex> lock(mtx);
    mTaskQueue.push(std::move(file));
    notify();
    if (!isThreadRunning()) startThread();
}

void TimeDomainSpecInsert::run() {
    while (!threadShouldExit()) {
        std::string filePath;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (mTaskQueue.empty()) {
                wait(-1);
                continue;
            }
            filePath = std::move(mTaskQueue.front());
            mTaskQueue.pop();
        }
        processSingleFile(filePath);
    }
}

void TimeDomainSpecInsert::processSingleFile(std::string file) {
    int result{0}; // 解码层结果，一般成功返回零
    AVFormatContext* inputContext{nullptr};
    avformat_open_input(&inputContext, file.c_str(), nullptr, nullptr);
    avformat_find_stream_info(inputContext, nullptr);

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];

    // 时长（秒）
    double duration{0.000001f}; // 防止失败的时候除以零
    if (inputContext->duration != AV_NOPTS_VALUE) {
        duration = static_cast<double>(pAudioStream->duration) / AV_TIME_BASE;
    }
    auto* decoderPar = pAudioStream->codecpar;

    auto codec = avcodec_find_decoder(decoderPar->codec_id);   // 根据ID寻找解码器
    auto* decoderContext = avcodec_alloc_context3(codec);      // 分配解码器上下文空间
    avcodec_parameters_to_context(decoderContext, decoderPar); // 连接参数和上下文
    avcodec_open2(decoderContext, codec, nullptr);             // 解码器初始化

    SwrContext* swrContext{nullptr};
    AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
    swr_alloc_set_opts2( // 下面初始化重采样器
            &swrContext, 
            &mono,//单通道 
            AV_SAMPLE_FMT_FLTP,//float 且平面结构 
            decoderPar->sample_rate, 
            &decoderPar->ch_layout,
            static_cast<AVSampleFormat>(decoderPar->format), 
            decoderPar->sample_rate, 
            0, 
            nullptr
        );
    swr_init(swrContext);

    juce::AudioBuffer<float> buffer;

    int64_t totalSamples = static_cast<int64_t>(duration * (double)decoderPar->sample_rate);

    // 前端显示128根柱子
    buffer.setSize(1, 128);
    auto samplePerLeftBin = static_cast<int64_t>(totalSamples / 128); // 每根柱子容纳多少个采样点
    auto samplePerRightBin{
        samplePerLeftBin + 1
    }; // 靠右的格子为靠左的加一，因为总数可能不会被128整除
    auto num4LeftBin{totalSamples - 128 * samplePerLeftBin}; // 左边柱子的数目

    buffer.clear();
    int binCount{0};        // 帧循环中使用了多少根柱子
    int sampleCount{0};     // 每个柱子中已经存储了多少个样本点
    float squarePlus{0.0f}; // 平方和

    auto pushDataIntoAudioGraph = [&](uint8_t* outputArray, int swrResult) {
        auto* dest = buffer.getWritePointer(0);
        const float* src = reinterpret_cast<const float*>(outputArray);
        for (int i = 0; i < swrResult; i++) {
            if (binCount >= 128) return;
            auto square = src[i] * src[i];
            squarePlus += square;
            sampleCount++;
            if (binCount < num4LeftBin) {
                if (sampleCount >= samplePerLeftBin) {

                    sampleCount = 0;

                    auto rms{std::sqrt(squarePlus / samplePerLeftBin)};
                    dest[binCount] = rms;
                    binCount++;
                    squarePlus = 0.0f;
                }
            } else {
                if (sampleCount >= samplePerRightBin) {
                    sampleCount = 0;

                    auto rms{std::sqrt(squarePlus / samplePerRightBin)};
                    dest[binCount] = rms;
                    binCount++;
                    squarePlus = 0.0f;
                }
            }
        }
    };

    auto* packet = av_packet_alloc();
    auto* frame = av_frame_alloc();

    while (av_read_frame(inputContext, packet) == 0) {

        if (packet->stream_index != currentIndex) {
            av_packet_unref(packet);
            continue;
        }
        result = avcodec_send_packet(decoderContext, packet);
        av_packet_unref(packet);
        if (result != 0) continue;
        while (avcodec_receive_frame(decoderContext, frame) == 0) {
            auto numOutputSamples =
                swr_get_delay(swrContext, decoderPar->sample_rate) + frame->nb_samples;

            uint8_t* outputArray{nullptr};
            result = av_samples_alloc(
                &outputArray, // 输出通道指针数组的起始地址
                nullptr,      // 物理字节大小
                1,
                numOutputSamples,
                AV_SAMPLE_FMT_FLTP,
                0
            );

            if (result < 0) {
                av_frame_unref(frame);
                av_freep(&outputArray);
                continue;
            }

            int num4SwrSamples = swr_convert(
                swrContext,
                &outputArray,     // 这里面装的就是重采样后的PCM数据
                numOutputSamples, // 输出缓冲区的最大容量
                static_cast<uint8_t**>(frame->extended_data),
                frame->nb_samples
            ); // 执行重采样，返回实际重采样完的样本点个数

            if (num4SwrSamples < 0) {
                av_frame_unref(frame);
                av_freep(&outputArray);
                continue;
            }

            av_frame_unref(frame);

            if (result <= 0) {
                av_freep(&outputArray);
                continue;
            }

            pushDataIntoAudioGraph(outputArray, num4SwrSamples);

            av_freep(&outputArray);
        }
    }

    // 第一次解码器flush
    avcodec_send_packet(decoderContext, nullptr);
    while (avcodec_receive_frame(decoderContext, frame) == 0) {
        auto delayNumSamples = swr_get_delay(swrContext, frame->sample_rate);
        int numOutputSamples = delayNumSamples + frame->nb_samples;

        uint8_t* outputArray{nullptr};
        result = av_samples_alloc(
            &outputArray, // 输出通道指针数组的起始地址
            nullptr,      // 物理字节大小
            1,
            numOutputSamples,
            AV_SAMPLE_FMT_FLTP,
            0
        ); // 第二个参数和第四个参数的区别：nb_samples 是逻辑样本数（比如 1024 个浮点样本）。
        // linesize 是物理字节大小（比如对于 1024 个浮点样本，linesize 通常是 1024 * 4 = 4096
        // 字节，但如果内存对齐强制要求 64 字节对齐，它可能是 4096 或 4096+）。

        if (result < 0) {
            av_frame_unref(frame);
            av_freep(&outputArray);
            continue;
        }

        result = swr_convert(
            swrContext,
            &outputArray,     // 这里面装的就是重采样后的PCM数据
            numOutputSamples, // 输出缓冲区的最大容量(理论最大值)
            static_cast<uint8_t**>(frame->extended_data),
            frame->nb_samples
        ); // 执行重采样，返回实际重采样完的样本点个数

        av_frame_unref(frame); // 这时候frame已经没有用了
        // 这个函数的目的是清空重置，而av_frame_free的作用是彻底删除
        // 前面那个可以理解做清空数组，后面那个意味着连数组的内存也一起销毁

        if (result <= 0) {
            av_freep(&outputArray);
            continue;
        }

        pushDataIntoAudioGraph(outputArray, result);

        av_freep(&outputArray);
    }

    // 第二次重采样器flush
    int maxFlushSamples = swr_get_out_samples(swrContext, 0);
    // 参数 0 表示“不喂入任何新输入，只计算内部残余需要多少输出空间”

    if (maxFlushSamples > 0) {
        uint8_t* outputArray = nullptr;
        av_samples_alloc(&outputArray, nullptr, 1, maxFlushSamples, AV_SAMPLE_FMT_FLTP, 0);

        while (true) {
            int ret = swr_convert(swrContext, &outputArray, maxFlushSamples, nullptr, 0);
            if (ret <= 0) break;
            pushDataIntoAudioGraph(outputArray, ret);
        }
        // 释放 Flush 临时缓冲区
        if (outputArray != nullptr) {
            av_freep(&outputArray);
        }
    }
    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swrContext);
    avcodec_free_context(&decoderContext);

    try {
        SQLite::Statement sql(
            mDb,
            "UPDATE songs SET timeDomainSpec = :timeDomainSpec WHERE filePath = :filePath"
        );
        sql.bind(
            ":timeDomainSpec",
            buffer.getReadPointer(0),
            static_cast<int>(buffer.getNumSamples() * sizeof(float))
        ); // 时域图
        sql.bind(":filePath", file);

        sql.exec();
        auto fileName{juce::File(file).getFileName()};
        auto cString{DllUtils::sendString2Frontend(fileName)};

        if (onFileTaskOver) onFileTaskOver(cString);
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("TimeDomainSpecInsert线程发生数据库错误:{}", e.what());
    }
}
TimeDomainSpecInsert::~TimeDomainSpecInsert() {
    stopThread(500); // stopThread里面实现了notify,所以不需要我再写一遍
}