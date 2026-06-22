#include "otherUtils.hpp"
#include "juce_core/juce_core.h"
#include <cstddef>
#include <iostream>

/*
 * 日志架构 (7 个日志文件)
 *
 * 文件                      默认级别  async?  用途
 * ───────────────────────────────────────────────────────────
 * player_audio.log          Error    是      FFmpeg 解码异常、音频图变更、VST XRun
 * player_scheduler.log      Info     否      入队/出队时间戳、慢查询(>50ms)、文件扫描耗时
 * player_ui.log             Info     否      切换播放模式、创建/删除歌单、HTTP 状态码
 * ai_worker.log             Info     是      AI Task Start/Finish、分轨进度%、ORT 错误码
 * vst_host.log              Warn     否      插件扫描成败、黑名单标记、NaN 参数告警
 * crash.log                 Fatal    否      未捕获异常、调用栈
 * all.log                   Debug    是      开发调试全量，release 关闭
 */

void logSystem::initLog()
{
    try
    {
        // ── 1. 准备日志目录 ──
        juce::File exeFile = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile);
        auto logDir = exeFile.getParentDirectory().getChildFile("logs");
        if (!logDir.exists())
            logDir.createDirectory();

        const auto logDirPath = logDir.getFullPathName().toStdString();

        // ── 2. 初始化异步日志线程池 ──
        // queue_size=8192 / 1 个后台线程，足以应对音频和 AI 的异步写入
        spdlog::init_thread_pool(8192, 1);//8192指的是队列最多能够容纳的消息条数

        // 全局日志格式：时间戳 + 级别 + 线程ID + 消息体
        const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";

        // ── 3. 按表创建 7 个日志器 ──

        // ── 0: player_audio.log — 音频流水线 (Error, async) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/player_audio.log", 0, 0);
            audioLogger = std::make_shared<spdlog::async_logger>(
                "audio", std::move(sink), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            audioLogger->set_pattern(pattern);
            audioLogger->set_level(spdlog::level::err);
            spdlog::register_logger(audioLogger);
        }

        // ── 1: player_scheduler.log — 多线程调度与队列 (Info) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/player_scheduler.log", 0, 0);
            schedulerLogger = std::make_shared<spdlog::logger>(
                "scheduler", std::move(sink));
            schedulerLogger->set_pattern(pattern);
            schedulerLogger->set_level(spdlog::level::info);
            spdlog::register_logger(schedulerLogger);
        }

        // ── 2: player_ui.log — 用户行为与操作 (Info) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/player_ui.log", 0, 0);
            uiLogger = std::make_shared<spdlog::logger>(
                "ui", std::move(sink));
            uiLogger->set_pattern(pattern);
            uiLogger->set_level(spdlog::level::info);
            spdlog::register_logger(uiLogger);
        }

        // ── 3: ai_worker.log — AI 跨进程 IPC (Info, async) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/ai_worker.log", 0, 0);
            aiLogger = std::make_shared<spdlog::async_logger>(
                "ai", std::move(sink), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            aiLogger->set_pattern(pattern);
            aiLogger->set_level(spdlog::level::info);
            spdlog::register_logger(aiLogger);
        }

        // ── 4: vst_host.log — VST/AU 插件宿主 (Warn) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/vst_host.log", 0, 0);
            vstLogger = std::make_shared<spdlog::logger>(
                "vst", std::move(sink));
            vstLogger->set_pattern(pattern);
            vstLogger->set_level(spdlog::level::warn);
            spdlog::register_logger(vstLogger);
        }

        // ── 5: crash.log — 全进程崩溃转储 (Fatal) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/crash.log", 0, 0);
            crashLogger = std::make_shared<spdlog::logger>(
                "crash", std::move(sink));
            crashLogger->set_pattern(pattern);
            crashLogger->set_level(spdlog::level::critical);
            // 崩溃日志每条立即刷盘
            crashLogger->flush_on(spdlog::level::info);
            spdlog::register_logger(crashLogger);
        }

        // ── 6: all.log — 全量汇聚 (Debug, async, release 关闭) ──
        {
            auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
                logDirPath + "/all.log", 0, 0);
            allLogger = std::make_shared<spdlog::async_logger>(
                "all", std::move(sink), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            allLogger->set_pattern(pattern);
#ifdef NDEBUG
            allLogger->set_level(spdlog::level::off);   // release 关闭
#else
            allLogger->set_level(spdlog::level::debug);  // debug 全量
#endif
            spdlog::register_logger(allLogger);
        }

        // ── 全局崩溃时刷盘策略 ──
        spdlog::flush_on(spdlog::level::critical);

        // 初始化确认
        if (allLogger && allLogger->should_log(spdlog::level::debug))
            allLogger->debug("日志系统初始化完成，共 {} 个日志器", numLogs);

    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "日志系统初始化失败 (spdlog): " << ex.what() << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "日志系统初始化失败: " << ex.what() << std::endl;
    }
}
