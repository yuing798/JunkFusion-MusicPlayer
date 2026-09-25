#pragma once
#include "juce_core/juce_core.h"
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

namespace OtherUtils {

    void initAudioLogger(juce::File cacheDir); // 初始化日志
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

} // namespace OtherUtils