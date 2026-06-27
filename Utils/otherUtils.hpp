#pragma once
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <string>

class logSystem
{
private:
    static constexpr size_t numLogs{7};

    // ── 日志器实例 ──
    // 0: player_audio.log  — 音频流水线 (Error, async)
    // 1: player_scheduler.log — 多线程调度与队列 (Info)
    // 2: player_ui.log     — 用户行为与操作 (Info)
    // 3: ai_worker.log     — AI 跨进程 IPC (Info, async)
    // 4: vst_host.log      — VST/AU 插件宿主 (Warn)
    // 5: crash.log         — 全进程崩溃转储 (Fatal)
    // 6: all.log           — 全量汇聚开发调试 (Debug, async, release 关闭)

    std::shared_ptr<spdlog::logger> audioLogger;
    std::shared_ptr<spdlog::logger> schedulerLogger;
    std::shared_ptr<spdlog::logger> uiLogger;
    std::shared_ptr<spdlog::logger> aiLogger;
    std::shared_ptr<spdlog::logger> vstLogger;
    std::shared_ptr<spdlog::logger> crashLogger;
    std::shared_ptr<spdlog::logger> allLogger;
public:
    logSystem();

    void init();
};

//输出错误码
std::string ffmpegErrorOutput(int result);
