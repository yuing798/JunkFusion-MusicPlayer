#include "otherUtils.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Exception.h>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>
#include <utility>
#include <zmq.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h> //负责日志信息
#include <libavutil/samplefmt.h>
}

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

// logSystem::logSystem()
//     : audioLogger(nullptr), schedulerLogger(nullptr), aiLogger(nullptr), vstLogger(nullptr),
//       crashLogger(nullptr), allLogger(nullptr) {}
// void logSystem::init() {
//     try {
//         // ── 1. 准备日志目录 ──

//         const auto logDirPath = logInfoDirId.getFullPathName().toStdString();

//         // ── 2. 初始化异步日志线程池 ──
//         // queue_size=8192 / 1 个后台线程，足以应对音频和 AI 的异步写入
//         spdlog::init_thread_pool(8192, 1); // 8192指的是队列最多能够容纳的消息条数

//         // 全局日志格式：时间戳 + 级别 + 线程ID + 消息体
//         const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";

//         // ── 3. 按表创建 7 个日志器 ──

//
//         constexpr size_t kSchedulerMaxSize = 5 * 1024 * 1024; // 5 MB
//         constexpr size_t kSchedulerMaxFiles = 3;

//         // ── 1: player_scheduler.log — 多线程调度与队列 (Info) ──
//         {
//             auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
//                 logDirPath + "/player_scheduler.log",
//                 kSchedulerMaxSize,
//                 kSchedulerMaxFiles
//             );
//             schedulerLogger = std::make_shared<spdlog::logger>(LogSchedulerID, std::move(sink));
//             schedulerLogger->set_pattern(pattern);
//             schedulerLogger->set_level(spdlog::level::info);
//             spdlog::register_logger(schedulerLogger);
//         }

//         constexpr size_t kAiMaxSize = 10 * 1024 * 1024; // 10 MB（AI 日志量大）
//         constexpr size_t kAiMaxFiles = 3;

//         // ── 3: ai_worker.log — AI 跨进程 IPC (Info, async) ──
//         {
//             auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
//                 logDirPath + "/ai_worker.log",
//                 kAiMaxSize,
//                 kAiMaxFiles
//             );
//             aiLogger = std::make_shared<spdlog::async_logger>(
//                 LogAiID,
//                 std::move(sink),
//                 spdlog::thread_pool(),
//                 spdlog::async_overflow_policy::block
//             );
//             aiLogger->set_pattern(pattern);
//             aiLogger->set_level(spdlog::level::info);
//             spdlog::register_logger(aiLogger);
//         }

//         constexpr size_t kVstMaxSize = 5 * 1024 * 1024; // 5 MB
//         constexpr size_t kVstMaxFiles = 3;

//         // ── 4: vst_host.log — VST/AU 插件宿主 (Warn) ──
//         {
//             auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
//                 logDirPath + "/vst_host.log",
//                 kVstMaxSize,
//                 kVstMaxFiles
//             );
//             vstLogger = std::make_shared<spdlog::logger>(LogVSTID, std::move(sink));
//             vstLogger->set_pattern(pattern);
//             vstLogger->set_level(spdlog::level::warn);
//             spdlog::register_logger(vstLogger);
//         }

//         constexpr size_t kCrashMaxSize = 2 * 1024 * 1024; // 2 MB
//         constexpr size_t kCrashMaxFiles = 5;              // 崩溃日志多留几份

//         // ── 5: crash.log — 全进程崩溃转储 (Fatal) ──
//         {
//             auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
//                 logDirPath + "/crash.log",
//                 kCrashMaxSize,
//                 kCrashMaxFiles
//             );
//             crashLogger = std::make_shared<spdlog::logger>(LogCrashID, std::move(sink));
//             crashLogger->set_pattern(pattern);
//             crashLogger->set_level(spdlog::level::critical);
//             // 崩溃日志每条立即刷盘
//             crashLogger->flush_on(spdlog::level::info);
//             spdlog::register_logger(crashLogger);
//         }

//         constexpr size_t kAllMaxSize = 20 * 1024 * 1024; // 20 MB（全量调试日志量大）
//         constexpr size_t kAllMaxFiles = 3;

//         // ── 6: all.log — 全量汇聚 (Debug, async, release 关闭) ──
//         {
//             auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
//                 logDirPath + "/all.log",
//                 kAllMaxSize,
//                 kAllMaxFiles
//             );
//             allLogger = std::make_shared<spdlog::async_logger>(
//                 LogAllID,
//                 std::move(sink),
//                 spdlog::thread_pool(),
//                 spdlog::async_overflow_policy::block
//             );
//             allLogger->set_pattern(pattern);
// #ifdef NDEBUG
//             allLogger->set_level(spdlog::level::off); // release 关闭
// #else
//             allLogger->set_level(spdlog::level::debug); // debug 全量
// #endif
//             spdlog::register_logger(allLogger);
//         }

//         // ── 全局崩溃时刷盘策略 ──
//         spdlog::flush_on(spdlog::level::critical);

//         // 初始化确认
//         if (allLogger && allLogger->should_log(spdlog::level::debug))
//             allLogger->debug("日志系统初始化完成，共 {} 个日志器", numLogs);

//     } catch (const spdlog::spdlog_ex& ex) {
//         std::cerr << "日志系统初始化失败 (spdlog): " << ex.what() << std::endl;
//     } catch (const std::exception& ex) {
//         std::cerr << "日志系统初始化失败: " << ex.what() << std::endl;
//     }
// }

std::string Utils::ffmpegErrorOutput(int result) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    // 将错误码ret转换为可读字符串存入errbuf
    av_strerror(result, errbuf, sizeof(errbuf));
    return std::string(errbuf);
}

std::vector<std::byte> Utils::loadFile2ByteVector(const juce::File& file) {
    // 确保文件真实存在
    if (!file.existsAsFile()) return {};

    // 创建 JUCE 的文件输入流
    auto stream = file.createInputStream();

    if (stream == nullptr || stream->failedToOpen()) return {};

    // 获取文件的总字节数
    const auto fileSize = static_cast<size_t>(stream->getTotalLength());

    // vector 的内存空间
    std::vector<std::byte> buffer(fileSize);

    // 直接读取到 vector 的物理内存首地址中
    // buffer.data() 返回 std::byte*，会自动隐式转换为 stream->read 索要的 void*
    stream->read(buffer.data(), static_cast<int>(fileSize));

    return buffer;
}

juce::DynamicObject::Ptr Utils::mb2object(const juce::MemoryBlock& mb) {
    // 1. 将 MemoryBlock 转换回 UTF-8 字符串
    juce::String jsonStr = mb.toString();

    // 2. 准备一个 juce::var 容器接收解析结果
    juce::var parsedJson;

    // 3. 使用带有 Result 返回值的 parse 函数进行安全解析
    juce::Result parseResult = juce::JSON::parse(jsonStr, parsedJson);
    if (parseResult.wasOk()) {
        if (parsedJson.isObject()) {
            return parsedJson.getDynamicObject();
        }
    }
    return {};
}

/**
 * @brief 紧急降级日志函数(绝不抛出异常，写完立刻刷盘）
 * @param message 待写入的日志消息字符串
 */
void Utils::writeEmergencyLog(std::string message) {
#ifdef JF_DEBUG
    std::ofstream testFile(
        "D:/audio_develop/Junk-Fusion/text.txt",
        std::ios::out | std::ios::app
    ); // 追加模式
    if (testFile.is_open()) {
        testFile << message.c_str() << std::endl;
        testFile.flush();
        testFile.close();
    }
#endif
}
void Utils::checkCurrentThreadId(std::string identity) {
    writeEmergencyLog((identity + " thread id: " +
                       std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())))
                          .c_str());
}

int Utils::levenshteinDistance(const juce::String& s1, const juce::String& s2) {
    const int len1 = s1.length();
    const int len2 = s2.length();
    std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1));

    for (int i = 0; i <= len1; ++i)
        dp[i][0] = i;
    for (int j = 0; j <= len2; ++j)
        dp[0][j] = j;

    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min(
                {dp[i - 1][j] + 1, // 删除
                 dp[i][j - 1] + 1, // 插入
                 dp[i - 1][j - 1] + cost}
            ); // 替换
        }
    }
    return dp[len1][len2];
}

// 计算相似度分数（0~1）
double Utils::stringSimilarity(const juce::String& s1, const juce::String& s2) {
    if (s1.isEmpty() && s2.isEmpty()) return 1.0;
    int maxLen = std::max(s1.length(), s2.length());
    if (maxLen == 0) return 1.0; // 两者都空
    int distance = levenshteinDistance(s1, s2);
    return 1.0 - static_cast<double>(distance) / maxLen;
}

Utils::Yvar Utils::Yvar::read(const char* key) const {
    if (!value.isObject()) {
        return {};
    }
    if (!value.hasProperty(key)) {
        return {};
    }
    return value[key];
}
Utils::Yvar Utils::Yvar::read(int index) const {
    if (!value.isArray()) {
        return {};
    }
    if (index < 0 || index >= value.size()) {
        return {};
    }
    return value[index];
}
double Utils::Yvar::toDouble() const {
    if (value.isDouble()) {
        return static_cast<double>(value);
    } else {
        return 0.0;
    }
}