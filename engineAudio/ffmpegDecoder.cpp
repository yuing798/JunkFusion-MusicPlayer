#include "./ffmpegDecoder.hpp"
#include "constants.h"
#include "dbManager.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <libavutil/channel_layout.h>
#include <spdlog/spdlog.h>

extern "C" {
#include "libavformat/avio.h"     //文件读取和输出
#include "libavutil/audio_fifo.h" //环形缓冲区提供
#include "libavutil/avutil.h"
#include "libswresample/swresample.h" //重采样
#include <libavcodec/avcodec.h>       //负责编解码
#include <libavformat/avformat.h>     // 负责解封装
#include <libavutil/log.h>            //负责日志信息
}

FFmpegDecoder::FFmpegDecoder(AudioRingBuffer& b) : juce::Thread("Decoder"), ringBuffer(b) {
    setPriority(juce::Thread::Priority::highest); // 最高优先级
}

FFmpegDecoder::~FFmpegDecoder() {
    // 给予线程 2000 毫秒的时间完成当前的 while 循环并退出。
    // 如果 2000ms 后仍未退出，系统会强制终止
    stopThread(2000);
}

void FFmpegDecoder::prepareToPlay(juce::AudioChannelSet layout, double s) {

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
}

void FFmpegDecoder::prepareToPlayNewSong(int64_t id) { currentSongId = id; }

void FFmpegDecoder::run() {
    // 这里可以放置只属于该线程的 FFmpeg 局部变量初始化逻辑...

#ifdef JUCE_DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_ERROR);
#endif

    AVFormatContext* inputContext;
    auto info{dbManager::getInstance().getSongsManager().getPlayInfoBySongId(currentSongId)};
    if (!info.has_value()) {
        // 弹出错误弹窗
        auto log = spdlog::get(LogAudioID);
        log->error("获取播放信息失败");
    }
    int result = avformat_open_input(&inputContext, info->path.c_str(), NULL, NULL);
    if (result < 0) {
        auto log = spdlog::get(LogAudioID);
        log->error("打开音频文件失败，通知用户检查原始文件:{},失败原因:{}", info->path,
                   ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return;
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if (result < 0) {
        auto log = spdlog::get(LogAudioID);
        log->error("获取流失败，通知用户检查原始文件:{},失败原因:{}", info->path,
                   ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return;
    }

    AVChannelLayout originalChannelLayout;

    if (info->originalChannelLayoutMask != 0) {
        av_channel_layout_from_mask(&originalChannelLayout, info->originalChannelLayoutMask);
    } else {
        av_channel_layout_default(&originalChannelLayout, info->originalNumChannels);
    }

    // JUCE 规范：必须使用 threadShouldExit() 作为死循环的唯一判断条件
    while (!threadShouldExit()) {
        // 一次解码大约需要 4096 个采样的空间
        if (ringBuffer.getFreeSpace() < 4096) {
            // 使用 wait() 而不是 sleep()
            // wait(10) 表示最多休眠 10 毫秒。如果这期间外部调用了 notify()，它会瞬间醒来。
            wait(10);
            continue;
        }

        // ====================================================
        // 执行 FFmpeg 解码与重采样逻辑
        // ====================================================
    }

    // 线程即将退出，这里可以放置 FFmpeg 上下文的安全销毁逻辑 (avcodec_free_context 等)
}