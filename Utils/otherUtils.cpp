#include "otherUtils.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include <SQLiteCpp/Exception.h>
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
extern "C"{
    #include <libavutil/error.h>//负责日志信息
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

logSystem::logSystem()
:audioLogger(nullptr),schedulerLogger(nullptr),uiLogger(nullptr),
aiLogger(nullptr),vstLogger(nullptr),crashLogger(nullptr),allLogger(nullptr){
    
}
void logSystem::init(){
    try
    {
        // ── 1. 准备日志目录 ──
        auto logDir = logInfoDirId.getChildFile("logs");
        if (!logDir.exists())
            logDir.createDirectory();

        const auto logDirPath = logDir.getFullPathName().toStdString();

        // ── 2. 初始化异步日志线程池 ──
        // queue_size=8192 / 1 个后台线程，足以应对音频和 AI 的异步写入
        spdlog::init_thread_pool(8192, 1);//8192指的是队列最多能够容纳的消息条数

        // 全局日志格式：时间戳 + 级别 + 线程ID + 消息体
        const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";

        // ── 3. 按表创建 7 个日志器 ──

        constexpr size_t kAudioMaxSize  = 5 * 1024 * 1024;   // 5 MB
        constexpr size_t kAudioMaxFiles = 3;

        // ── 0: player_audio.log — 音频流水线 (Error, async) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/player_audio.log", kAudioMaxSize, kAudioMaxFiles);
            audioLogger = std::make_shared<spdlog::async_logger>(
                LogAudioID, std::move(sink), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            audioLogger->set_pattern(pattern);
            audioLogger->set_level(spdlog::level::err);
            spdlog::register_logger(audioLogger);
        }

        constexpr size_t kSchedulerMaxSize  = 5 * 1024 * 1024;   // 5 MB
        constexpr size_t kSchedulerMaxFiles = 3;

        // ── 1: player_scheduler.log — 多线程调度与队列 (Info) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/player_scheduler.log", kSchedulerMaxSize, kSchedulerMaxFiles);
            schedulerLogger = std::make_shared<spdlog::logger>(
                LogSchedulerID, std::move(sink));
            schedulerLogger->set_pattern(pattern);
            schedulerLogger->set_level(spdlog::level::info);
            spdlog::register_logger(schedulerLogger);
        }

        constexpr size_t kUiMaxSize  = 5 * 1024 * 1024;   // 5 MB
        constexpr size_t kUiMaxFiles = 3;

        // ── 2: player_ui.log — 用户行为与操作 (Info) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/player_ui.log", kUiMaxSize, kUiMaxFiles);
            uiLogger = std::make_shared<spdlog::logger>(
                LogUiID, std::move(sink));
            uiLogger->set_pattern(pattern);
            uiLogger->set_level(spdlog::level::info);
            spdlog::register_logger(uiLogger);
        }

        constexpr size_t kAiMaxSize  = 10 * 1024 * 1024;  // 10 MB（AI 日志量大）
        constexpr size_t kAiMaxFiles = 3;

        // ── 3: ai_worker.log — AI 跨进程 IPC (Info, async) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/ai_worker.log", kAiMaxSize, kAiMaxFiles);
            aiLogger = std::make_shared<spdlog::async_logger>(
                LogAiID, std::move(sink), spdlog::thread_pool(),
                spdlog::async_overflow_policy::block);
            aiLogger->set_pattern(pattern);
            aiLogger->set_level(spdlog::level::info);
            spdlog::register_logger(aiLogger);
        }

        constexpr size_t kVstMaxSize  = 5 * 1024 * 1024;   // 5 MB
        constexpr size_t kVstMaxFiles = 3;

        // ── 4: vst_host.log — VST/AU 插件宿主 (Warn) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/vst_host.log", kVstMaxSize, kVstMaxFiles);
            vstLogger = std::make_shared<spdlog::logger>(
                LogVSTID, std::move(sink));
            vstLogger->set_pattern(pattern);
            vstLogger->set_level(spdlog::level::warn);
            spdlog::register_logger(vstLogger);
        }

        constexpr size_t kCrashMaxSize  = 2 * 1024 * 1024;   // 2 MB
        constexpr size_t kCrashMaxFiles = 5;                 // 崩溃日志多留几份

        // ── 5: crash.log — 全进程崩溃转储 (Fatal) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/crash.log", kCrashMaxSize, kCrashMaxFiles);
            crashLogger = std::make_shared<spdlog::logger>(
                LogCrashID, std::move(sink));
            crashLogger->set_pattern(pattern);
            crashLogger->set_level(spdlog::level::critical);
            // 崩溃日志每条立即刷盘
            crashLogger->flush_on(spdlog::level::info);
            spdlog::register_logger(crashLogger);
        }

        constexpr size_t kAllMaxSize  = 20 * 1024 * 1024;  // 20 MB（全量调试日志量大）
        constexpr size_t kAllMaxFiles = 3;

        // ── 6: all.log — 全量汇聚 (Debug, async, release 关闭) ──
        {
            auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                logDirPath + "/all.log", kAllMaxSize, kAllMaxFiles);
            allLogger = std::make_shared<spdlog::async_logger>(
                LogAllID, std::move(sink), spdlog::thread_pool(),
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

std::string ffmpegErrorOutput(int result){
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0}; 
    // 将错误码ret转换为可读字符串存入errbuf
    av_strerror(result, errbuf, sizeof(errbuf));
    return std::string(errbuf); 
}

// std::vector<std::byte> loadFile2ByteVector (const juce::File& file)
// {
//     //确保文件真实存在
//     if (!file.existsAsFile()) return {};

//     //创建 JUCE 的文件输入流
//     auto stream = file.createInputStream();
    
//     if (stream == nullptr || stream->failedToOpen())
//         return {};

//     // 获取文件的总字节数
//     const auto fileSize = static_cast<size_t> (stream->getTotalLength());
    
//     // vector 的内存空间
//     std::vector<std::byte> buffer (fileSize);

//     // 直接读取到 vector 的物理内存首地址中
//     // buffer.data() 返回 std::byte*，会自动隐式转换为 stream->read 索要的 void*
//     stream->read (buffer.data(), static_cast<int> (fileSize));

//     return buffer;
// }

void getMultiMediaFileChoose(std::function<void(const juce::Array<juce::File>&)> onFileSelected,juce::Component* parentComponent)
{
    // 1. 构建过滤器字符串（用分号分隔）
    juce::String filters = "*";

    // 2. 创建 FileChooser 对象（使用 shared_ptr 管理生命周期）
    auto chooser = std::make_shared<juce::FileChooser>(
        juce::String::fromUTF8("请选择多媒体文件（音频或视频）"),                // 对话框标题
        juce::File::getSpecialLocation(juce::File::userHomeDirectory), // 初始目录
        filters,                                         // 过滤器字符串
        true,                                            // 使用原生对话框（外观更好）
        false,                                           // 不将包视为目录
        parentComponent                                  // 父组件（实现模态）
    );

    // 3. 异步启动对话框
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems, // 支持多选
        [chooser, onFileSelected](const juce::FileChooser&)
            {

                juce::Array<juce::File> selected = chooser->getResults();

                // 调用回调，传递文件列表
                if (onFileSelected){//这个地方是在检查 std::function 这个“对象”是否为空
                    onFileSelected(selected);
                }
            }
    );
}