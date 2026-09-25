#include "./otherUtils.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
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

void OtherUtils::initAudioLogger(juce::File cacheDir) {
    std::shared_ptr<spdlog::logger> audioLogger;
    constexpr size_t kAudioMaxSize = 5 * 1024 * 1024; // 5 MB
    constexpr size_t kAudioMaxFiles = 3;
    auto logDir{cacheDir.getChildFile("log")};
    if (!logDir.exists()) logDir.createDirectory();
    auto logFile{logDir.getChildFile("audioProcess.log").getFullPathName().toStdString()};

    // 初始化 spdlog 的全局静态异步线程池，而不是使用局部变量
    spdlog::init_thread_pool(8192, 1);

    {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logFile,
            kAudioMaxSize,
            kAudioMaxFiles
        );
        audioLogger = std::make_shared<spdlog::async_logger>(
            LogAudioID,
            std::move(sink),
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block
        );
        audioLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
#ifdef JF_DEBUG
        audioLogger->set_level(spdlog::level::debug);
#else
        audioLogger->set_level(spdlog::level::err);
#endif
        audioLogger->flush_on(spdlog::level::err); // 遇到错误立刻刷盘
        spdlog::register_logger(audioLogger);
    }
    spdlog::get(LogAudioID)->debug("音频进程日志初始化完成");
}

std::string OtherUtils::ffmpegErrorOutput(int result) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    // 将错误码ret转换为可读字符串存入errbuf
    av_strerror(result, errbuf, sizeof(errbuf));
    return std::string(errbuf);
}

std::vector<std::byte> OtherUtils::loadFile2ByteVector(const juce::File& file) {
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

juce::DynamicObject::Ptr OtherUtils::mb2object(const juce::MemoryBlock& mb) {
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
void OtherUtils::writeEmergencyLog(std::string message) {
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
void OtherUtils::checkCurrentThreadId(std::string identity) {
    writeEmergencyLog((identity + " thread id: " +
                       std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())))
                          .c_str());
}

int OtherUtils::levenshteinDistance(const juce::String& s1, const juce::String& s2) {
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
double OtherUtils::stringSimilarity(const juce::String& s1, const juce::String& s2) {
    if (s1.isEmpty() && s2.isEmpty()) return 1.0;
    int maxLen = std::max(s1.length(), s2.length());
    if (maxLen == 0) return 1.0; // 两者都空
    int distance = levenshteinDistance(s1, s2);
    return 1.0 - static_cast<double>(distance) / maxLen;
}
