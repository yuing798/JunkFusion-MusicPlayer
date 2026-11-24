#include "./PluginPreflightRunner.hpp"

#include "Utils/constants.h"

#include <spdlog/spdlog.h>

namespace PluginHost {

    PluginPreflightRunner::PluginPreflightRunner(juce::File scanExe)
        : mScanExe(std::move(scanExe)) {}

    bool PluginPreflightRunner::isAvailable() const {
        return mScanExe.existsAsFile();
    }

    PluginPreflightRunner::Result PluginPreflightRunner::run(
        const juce::File& pluginFile,
        const std::string& identifier,
        double sampleRate,
        int blockSize,
        int timeoutMs
    ) {
        Result result;

        auto logger = spdlog::get(LogDllID);

        if (!isAvailable()) {
            // 没有扫描子进程就不做预检：放行，但记一条警告。
            // 这里刻意不"因为没预检就拒绝插入"——那样在没打包 scan exe 的
            // 开发环境里连插件都用不了了。
            result.ok = true;
            result.error = "预检进程不存在，跳过预检";
            if (logger) logger->warn("[Preflight] 跳过：找不到 {}", mScanExe.getFullPathName().toStdString());
            return result;
        }

        const auto reportFile = juce::File::createTempFile(".json");
        if (reportFile == juce::File{}) {
            result.error = "建不了预检报告临时文件";
            return result;
        }

        juce::StringArray args;
        args.add(mScanExe.getFullPathName());
        args.add("--preflight");
        args.add("--file");
        args.add(pluginFile.getFullPathName());
        args.add("--report");
        args.add(reportFile.getFullPathName());
        args.add("--sample-rate");
        args.add(juce::String(sampleRate, 3));
        args.add("--block");
        args.add(juce::String(blockSize));
        if (!identifier.empty()) {
            args.add("--identifier");
            args.add(juce::String::fromUTF8(identifier.c_str()));
        }

        juce::ChildProcess child;
        // streamFlags = 0：不建管道（子进程自己把 stdout/stderr 丢到 NUL）
        if (!child.start(args, 0)) {
            result.error = "拉不起预检进程";
            reportFile.deleteFile();
            return result;
        }

        const auto deadline = juce::Time::currentTimeMillis() + timeoutMs;
        bool finished = false;
        while (juce::Time::currentTimeMillis() < deadline) {
            if (child.waitForProcessToFinish(60)) {
                finished = true;
                break;
            }
        }

        if (!finished) {
            // 挂死：杀掉，按"超时"处理（报告文件可能还没写）
            child.kill();
            result.timedOut = true;
            result.error = "预检超时（" + std::to_string(timeoutMs / 1000) + " 秒没跑完）";
            if (logger)
                logger->warn(
                    "[Preflight] {} 超时",
                    pluginFile.getFileName().toStdString()
                );
        }

        // 不管是正常结束、崩溃还是超时，都先看有没有报告：
        // 报告是**在析构之前**写的，所以"报告存在"就意味着插件本体是好的
        const juce::String text = reportFile.existsAsFile() ? reportFile.loadFileAsString() : "";
        reportFile.deleteFile();

        if (text.isEmpty()) {
            if (!result.timedOut) {
                result.crashed = true;
                result.error = "预检进程没写出报告就退出了（大概率崩溃）";
            }
            if (logger)
                logger->warn(
                    "[Preflight] {} 失败：{}",
                    pluginFile.getFileName().toStdString(),
                    result.error
                );
            return result;
        }

        const auto parsed = juce::JSON::parse(text);
        if (auto* obj = parsed.getDynamicObject()) {
            result.ok = static_cast<bool>(obj->getProperty("ok"));
            result.error = obj->getProperty("error").toString().toStdString();
            result.name = obj->getProperty("name").toString().toStdString();
            result.inputs = static_cast<int>(obj->getProperty("inputs"));
            result.outputs = static_cast<int>(obj->getProperty("outputs"));
            result.latency = static_cast<int>(obj->getProperty("latency"));
            result.params = static_cast<int>(obj->getProperty("params"));
            result.hasEditor = static_cast<bool>(obj->getProperty("hasEditor"));
        } else {
            result.error = "预检报告不是合法 JSON";
        }

        if (logger) {
            logger->info(
                "[Preflight] {} -> ok={} {} ({} in / {} out, 延迟 {} 采样, {} 参数)",
                pluginFile.getFileName().toStdString(),
                result.ok,
                result.error,
                result.inputs,
                result.outputs,
                result.latency,
                result.params
            );
        }

        return result;
    }

} // namespace PluginHost
