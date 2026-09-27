#include "./WaveFormAnaly.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <array>
#include <chromaprint.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

WaveFormAnaly::WaveFormAnaly(SQLite::Database& db)
    : juce::Thread("WaveFormAnaly"), mDb(db), mOnlineGetMatedata(db) {}

void WaveFormAnaly::setTask(WaveFormAnaly::Task task) {
    std::lock_guard<std::mutex> lock(mtx);
    mTaskQueue.push(std::move(task));
    notify();
    if (!isThreadRunning()) startThread();
}

void WaveFormAnaly::run() {
    while (!threadShouldExit()) {
        Task task;
        bool hasTask = false;

        // 1. 缩小锁的作用域，仅在弹出任务时持锁
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!mTaskQueue.empty()) {
                task = std::move(mTaskQueue.front());
                mTaskQueue.pop();
                hasTask = true;
            }
        } // 锁在此处自动释放

        // 2. 根据是否有任务决定处理还是休眠
        if (hasTask) {
            processSingleFile(task); // 在锁外执行耗时任务
        } else {
            // 在【无锁状态】下安全挂起等待新任务！
            wait(-1);
        }
    }
}

void WaveFormAnaly::processSingleFile(WaveFormAnaly::Task task) {

    int result{0}; // 解码层结果，一般成功返回零
    av_log_set_level(AV_LOG_ERROR);
    AVFormatContext* inputContext{nullptr};
    avformat_open_input(&inputContext, task.path.c_str(), nullptr, nullptr);
    avformat_find_stream_info(inputContext, nullptr);

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];

    // 时长（秒）
    double duration{0.0f};
    if (pAudioStream->duration != AV_NOPTS_VALUE) {
        duration = pAudioStream->duration * av_q2d(pAudioStream->time_base);
    } else if (inputContext->duration != AV_NOPTS_VALUE) {
        duration = inputContext->duration / (double)AV_TIME_BASE;
    }
    auto* decoderPar = pAudioStream->codecpar;

    auto codec = avcodec_find_decoder(decoderPar->codec_id);   // 根据ID寻找解码器
    auto* decoderContext = avcodec_alloc_context3(codec);      // 分配解码器上下文空间
    avcodec_parameters_to_context(decoderContext, decoderPar); // 连接参数和上下文
    avcodec_open2(decoderContext, codec, nullptr);             // 解码器初始化

    SwrContext* swrContext{nullptr};
    AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
    const int targetSampleRate{11025};
    swr_alloc_set_opts2( // 下面初始化重采样器
            &swrContext, 
            &mono,//单通道 
            AV_SAMPLE_FMT_S16P,
            targetSampleRate, //Chromaprint 输出音频采样率必须为11025
            &decoderPar->ch_layout,
            static_cast<AVSampleFormat>(decoderPar->format), 
            decoderPar->sample_rate, 
            0, 
            nullptr
        );
    swr_init(swrContext);

    std::array<double, 128> blobBuffer{}; // 存入数据库的波形图数组,前端显示128根柱子

    int64_t totalSamples{static_cast<int64_t>(duration * (double)targetSampleRate)};

    auto samplePerLeftBin{static_cast<int64_t>(totalSamples / 128)}; // 每根柱子容纳多少个采样点
    auto samplePerRightBin{
        samplePerLeftBin + 1
    }; // 靠右的格子为靠左的加一，因为总数可能不会被128整除
    auto num4LeftBin{samplePerRightBin * 128 - totalSamples}; // 左边柱子的数目

    int binCount{0};         // 帧循环中使用了多少根柱子
    int sampleCount{0};      // 每个柱子中已经存储了多少个样本点
    double squarePlus{0.0f}; // 平方和

    auto pushDataIntoAudioGraph = [&](uint8_t* outputArray, int swrResult) {
        const int16_t* src = reinterpret_cast<const int16_t*>(outputArray);
        for (int i = 0; i < swrResult; i++) {
            if (binCount >= 128) return;
            double value{(double)src[i] / 32767.0};
            auto square = value * value;
            squarePlus += square;
            sampleCount++;
            if (binCount < num4LeftBin) {
                if (sampleCount >= samplePerLeftBin) {

                    sampleCount = 0;

                    auto rms{std::sqrt(squarePlus / samplePerLeftBin)};
                    blobBuffer[binCount] = rms;
                    binCount++;
                    squarePlus = 0.0f;
                }
            } else {
                if (sampleCount >= samplePerRightBin) {
                    sampleCount = 0;

                    auto rms{std::sqrt(squarePlus / samplePerRightBin)};
                    blobBuffer[binCount] = rms;
                    binCount++;
                    squarePlus = 0.0f;
                }
            }
        }
    };

    auto* packet = av_packet_alloc();
    auto* frame = av_frame_alloc();

    bool needOnlineSearch{
        task.onlineTask.album.isEmpty() || task.onlineTask.needCover ||
        task.onlineTask.artists.isEmpty() || task.onlineTask.title.isEmpty()
    }; // 是否需要联网搜索,四个里面有一个不存在就需要

    // bool needOnlineSearch{true}; // 这个测试用

    ChromaprintContext* printContext{nullptr};
    if (needOnlineSearch) {
        // 音频指纹提取初始化
        printContext = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
        chromaprint_start(printContext, targetSampleRate, 1); // 指纹提取强制这个格式
    }
    double currentSeconds{0.0}; // 指纹提取只提取前90秒

    while (av_read_frame(inputContext, packet) == 0) {

        if (packet->stream_index != currentIndex) {
            av_packet_unref(packet);
            continue;
        }
        result = avcodec_send_packet(decoderContext, packet);
        av_packet_unref(packet);
        if (result != 0) continue;
        while (avcodec_receive_frame(decoderContext, frame) == 0) {
            auto numInputSamples{
                swr_get_delay(swrContext, decoderPar->sample_rate) + frame->nb_samples
            };

            auto numOutputSamples{av_rescale_rnd(
                numInputSamples,
                targetSampleRate,
                decoderPar->sample_rate,
                AV_ROUND_UP
            )};

            uint8_t* outputArray{nullptr};
            result = av_samples_alloc(
                &outputArray, // 输出通道指针数组的起始地址
                nullptr,      // 物理字节大小
                1,
                numOutputSamples,
                AV_SAMPLE_FMT_S16P, // 指纹提取要求单通道且原始PCM格式
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

            pushDataIntoAudioGraph(outputArray, num4SwrSamples);

            currentSeconds += (double)frame->nb_samples / (double)decoderPar->sample_rate;

            av_frame_unref(frame);

            // 喂入指纹上下文
            if (needOnlineSearch && currentSeconds < 90.0)
                chromaprint_feed(printContext, (int16_t*)outputArray, num4SwrSamples);

            av_freep(&outputArray);
        }
    }

    // 第一次解码器flush
    avcodec_send_packet(decoderContext, nullptr);
    while (avcodec_receive_frame(decoderContext, frame) == 0) {
        auto numInputSamples{
            swr_get_delay(swrContext, decoderPar->sample_rate) + frame->nb_samples
        };

        auto numOutputSamples{
            av_rescale_rnd(numInputSamples, targetSampleRate, decoderPar->sample_rate, AV_ROUND_UP)
        };

        uint8_t* outputArray{nullptr};
        result = av_samples_alloc(
            &outputArray, // 输出通道指针数组的起始地址
            nullptr,      // 物理字节大小
            1,
            numOutputSamples,
            AV_SAMPLE_FMT_S16P,
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

        if (result <= 0) {
            av_freep(&outputArray);
            continue;
        }

        pushDataIntoAudioGraph(outputArray, result);
        currentSeconds += (double)frame->nb_samples / (double)decoderPar->sample_rate;
        av_frame_unref(frame);
        if (needOnlineSearch && currentSeconds < 90.0)
            chromaprint_feed(printContext, (int16_t*)outputArray, result);

        av_freep(&outputArray);
    }

    // 第二次重采样器flush
    int maxFlushSamples = swr_get_out_samples(swrContext, 0);
    // 参数 0 表示“不喂入任何新输入，只计算内部残余需要多少输出空间”

    if (maxFlushSamples > 0) {
        uint8_t* outputArray = nullptr;
        av_samples_alloc(&outputArray, nullptr, 1, maxFlushSamples, AV_SAMPLE_FMT_S16P, 0);

        while (true) {
            int ret = swr_convert(swrContext, &outputArray, maxFlushSamples, nullptr, 0);
            if (ret <= 0) break;
            pushDataIntoAudioGraph(outputArray, ret);

            currentSeconds += (double)ret / (double)decoderPar->sample_rate;
            if (needOnlineSearch && currentSeconds < 90.0)
                chromaprint_feed(printContext, (int16_t*)outputArray, ret);
        }
        // 释放 Flush 临时缓冲区
        if (outputArray != nullptr) {
            av_freep(&outputArray);
        }
    }

    if (binCount < 128 && sampleCount > 0) {
        // 此时 squarePlus 里面装的就是最后剩下的那些样本的平方和
        blobBuffer[binCount] = std::sqrt(squarePlus / sampleCount);
        binCount++;
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swrContext);
    avcodec_free_context(&decoderContext);

    if (needOnlineSearch) {
        chromaprint_finish(printContext);

        char* printPtr{nullptr};
        if (chromaprint_get_fingerprint(printContext, &printPtr) == 1) {
            std::string printStr{printPtr};
            chromaprint_dealloc(printPtr);
            task.onlineTask.duration = duration;
            task.onlineTask.print = std::move(printStr);
            mOnlineGetMatedata.setTask(std::move(task.onlineTask));
        }
        chromaprint_free(printContext);
    }

    try {
        SQLite::Statement sql(
            mDb,
            "UPDATE songs SET timeDomainSpec = :timeDomainSpec WHERE filePath = :filePath"
        );
        sql.bind(
            ":timeDomainSpec",
            blobBuffer.data(),
            static_cast<int>(blobBuffer.size() * sizeof(double))
        ); // 时域图
        sql.bind(":filePath", task.path);

        // std::string spdlogStr{""};
        // spdlog::get(LogDllID)->debug(
        //     "文件路径:{},波形图数组元素个数为:{},数组占用字节大小为{}",
        //     task.path,
        //     blobBuffer.size(),
        //     static_cast<int>(blobBuffer.size() * sizeof(double))
        // );
        // for (size_t i = 0; i < blobBuffer.size(); i++) {

        //     spdlogStr += std::to_string(blobBuffer[i]) + " ";
        // }
        // spdlog::get(LogDllID)->debug("波形图内容:{}", spdlogStr);

        sql.exec();
        // auto fileName{juce::File(task.path).getFileName()};
        // auto cString{ConvertUtils::copyStringOnHeap(fileName)};

        // if (onTimeDomainSpecInsertOver) onTimeDomainSpecInsertOver(cString);
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("WaveFormAnaly线程发生数据库错误:{}", e.what());
    }
}
WaveFormAnaly::~WaveFormAnaly() {
    stopThread(500); // stopThread里面实现了notify,所以不需要我再写一遍
}