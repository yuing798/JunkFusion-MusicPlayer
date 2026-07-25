#include "./ffmpegDecoder.hpp"
#include "constants.h"
#include "dbManager.hpp"
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

void FFmpegDecoder::prepareToPlay(int n, double s) {
    numChannels = n;
    sampleRate = s;
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

    AVChannelLayout outputChannelLayout;
    av_channel_layout_default(&outputChannelLayout, numChannels);
    AVChannelLayout originalChannelLayout;
    av_channel_layout_default(&originalChannelLayout, info->originalNumChannels);

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