#pragma once
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Exception.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include <zmq.hpp>

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

    // 将内存块转换成对象
    juce::DynamicObject::Ptr mb2object(const juce::MemoryBlock& mb);

    void writeEmergencyLog(std::string message);

    void checkCurrentThreadId(std::string identity); // 检查当前所在的线程ID号

    // 计算将一个字符串转换成另一个字符串所需的最少编辑操作次数（插入、删除、替换）。距离越小，字符串越相似。
    int levenshteinDistance(const juce::String& s1, const juce::String& s2);

    // 计算相似度分数（0 ~1）
    double stringSimilarity(const juce::String& s1, const juce::String& s2);

} // namespace Utils