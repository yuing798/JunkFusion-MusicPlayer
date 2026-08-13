#pragma once
#include "juce_core/juce_core.h"
#include "juce_graphics/juce_graphics.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <SQLiteCpp/Exception.h>
#include <cstdint>
#include <memory>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

// class logSystem {
// private:
//     static constexpr size_t numLogs{7};

//     // ── 日志器实例 ──
//     // 3: ai_worker.log     — AI 跨进程 IPC (Info, async)
//     // 4: vst_host.log      — VST/AU 插件宿主 (Warn)
//     // 5: crash.log         — 全进程崩溃转储 (Fatal)
//     // 6: all.log           — 全量汇聚开发调试 (Debug, async, release 关闭)

//     std::shared_ptr<spdlog::logger> schedulerLogger;
//     std::shared_ptr<spdlog::logger> aiLogger;
//     std::shared_ptr<spdlog::logger> vstLogger;
//     std::shared_ptr<spdlog::logger> crashLogger;
//     std::shared_ptr<spdlog::logger> allLogger;

// public:
//     logSystem();
//     // void sqlError(SQLite::Exception&);

//     void init();
// };

namespace Utils {
    // 输出错误码
    std::string ffmpegErrorOutput(int result);

    // 转化字符串为UTF8
    inline juce::String utf8(const char* name) { return juce::String::fromUTF8(name); };

    std::vector<std::byte> loadFile2ByteVector(const juce::File& file);

    juce::DynamicObject::Ptr mb2object(const juce::MemoryBlock& mb);
} // namespace Utils