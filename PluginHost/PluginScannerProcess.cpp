#include "./PluginScannerProcess.hpp"

#include "Utils/constants.h"

#include <spdlog/spdlog.h>

namespace PluginHost {

    PluginScannerProcess::PluginScannerProcess(Options options)
        : mOptions(std::move(options)) {
        mListFile = mOptions.workDir.getChildFile("scan_list.txt");
        mProgressFile = mOptions.workDir.getChildFile("scan_progress.txt");
    }

    bool PluginScannerProcess::writeFileList(const std::vector<juce::File>& modules) {
        if (!mOptions.workDir.isDirectory() && !mOptions.workDir.createDirectory()) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error(
                    "无法创建扫描工作目录:{}",
                    mOptions.workDir.getFullPathName().toStdString()
                );
            return false;
        }

        juce::StringArray lines;
        for (const auto& module : modules) {
            lines.add(module.getFullPathName());
        }

        // 一行一个绝对路径；子进程用 StringArray::fromLines 读回来，
        // index 就是这个列表的行号（断点续扫全靠它稳定）。
        return mListFile.replaceWithText(lines.joinIntoString("\n") + "\n");
    }

    bool PluginScannerProcess::startChild(juce::ChildProcess& child, int startIndex) {
        // 只认绝对路径：Coordinator 的工作目录是 Flutter 的 exe 目录，
        // 用相对路径在别的 IDE 里跑就会找不到。
        if (!mOptions.scanExe.existsAsFile()) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error(
                    "扫描进程不存在:{}",
                    mOptions.scanExe.getFullPathName().toStdString()
                );
            return false;
        }

        // 每次重开都先把进度文件删掉：否则新子进程还没写任何东西，
        // 主进程就会把上一轮的内容当成本轮的读进来（index 全错）。
        mProgressFile.deleteFile();

        juce::StringArray args;
        args.add(mOptions.scanExe.getFullPathName());
        args.add("--list");
        args.add(mListFile.getFullPathName());
        args.add("--start-index");
        args.add(juce::String(startIndex));
        args.add("--progress");
        args.add(mProgressFile.getFullPathName());

        // streamFlags = 0：不建管道。子进程自己把 stdout/stderr 丢到 NUL，
        // 结果走进度文件，所以这里不需要（也不能）读它的输出。
        return child.start(args, 0);
    }

    void PluginScannerProcess::handleLine(
        RunContext& ctx,
        const std::string& line,
        int64_t& lastEventMs
    ) {
        const auto event = decodeLine(line);
        if (event.kind == ScanEvent::Kind::unknown) return; // 第三方插件的噪声

        lastEventMs = juce::Time::currentTimeMillis();

        switch (event.kind) {
            case ScanEvent::Kind::ok: {
                if (event.index < 0 || event.index >= ctx.total) return;
                auto& result = (*ctx.results)[event.index];
                result.records.push_back(event.record);
                break;
            }
            case ScanEvent::Kind::end: {
                if (event.index < 0 || event.index >= ctx.total) return;
                auto& result = (*ctx.results)[event.index];
                result.ok = true;
                result.error.clear();

                // 只有"当前下标"结束才推进；重复/迟到的 END 不该把断点带跑偏
                if (event.index == ctx.index) ++ctx.index;
                break;
            }
            case ScanEvent::Kind::fail: {
                if (event.index >= 0 && event.index < ctx.total) {
                    markFailed(ctx, event.index, event.reason.empty() ? "扫描失败" : event.reason);
                }
                if (event.index == ctx.index) ++ctx.index;
                break;
            }
            case ScanEvent::Kind::done: {
                ctx.gotDone = true;
                break;
            }
            case ScanEvent::Kind::log: {
                if (auto logger = spdlog::get(LogDllID))
                    logger->debug("[PluginScan] {}", event.reason);
                break;
            }
            case ScanEvent::Kind::unknown:
                break;
        }

        if (ctx.onProgress != nullptr && *ctx.onProgress) {
            const auto current = (ctx.index < ctx.total) ? (*ctx.results)[ctx.index].filePath
                                                         : juce::String{};
            (*ctx.onProgress)(ctx.index, ctx.total, current);
        }
    }

    void PluginScannerProcess::readNewBytes(
        RunContext& ctx,
        int64_t& readPos,
        int64_t& lastEventMs
    ) {
        const auto fileSize = mProgressFile.getSize();
        if (fileSize <= readPos) return;

        juce::FileInputStream in{mProgressFile};
        if (!in.openedOk()) return;

        in.setPosition(readPos);

        // 按**字节**读，最后才 decode 成 UTF-8 字符串：
        // 一个中文字符有可能被读操作劈成两半，先拼成完整行再解码才不会花。
        char buffer[8192];
        int64_t remaining = fileSize - readPos;
        while (remaining > 0) {
            const int want = static_cast<int>(
                juce::jmin<int64_t>(remaining, static_cast<int64_t>(sizeof(buffer)))
            );
            const int got = in.read(buffer, want);
            if (got <= 0) break;

            readPos += got;
            remaining -= got;
            ctx.pendingBytes.append(buffer, static_cast<std::size_t>(got));
        }

        std::size_t begin = 0;
        while (true) {
            const auto newline = ctx.pendingBytes.find('\n', begin);
            if (newline == std::string::npos) break;

            std::string line = ctx.pendingBytes.substr(begin, newline - begin);
            begin = newline + 1;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) handleLine(ctx, line, lastEventMs);
        }

        ctx.pendingBytes.erase(0, begin);
    }

    void PluginScannerProcess::markFailed(
        RunContext& ctx,
        int index,
        const std::string& reason
    ) {
        if (index < 0 || index >= ctx.total) return;

        auto& result = (*ctx.results)[index];
        if (result.ok) return; // 已经有结果了，不要覆盖

        result.ok = false;
        result.error = reason;
        result.records.clear();
    }

    std::vector<PluginScannerProcess::FileResult> PluginScannerProcess::scan(
        const std::vector<juce::File>& modules,
        const ProgressFn& onProgress
    ) {
        std::vector<FileResult> results;
        results.reserve(modules.size());
        for (const auto& module : modules) {
            FileResult result;
            result.filePath = module.getFullPathName();
            results.push_back(std::move(result));
        }

        if (results.empty()) return results;

        if (!writeFileList(modules)) {
            for (auto& result : results) {
                result.ok = false;
                result.error = "无法写入扫描列表文件";
            }
            return results;
        }

        RunContext ctx;
        ctx.results = &results;
        ctx.total = static_cast<int>(results.size());
        ctx.onProgress = &onProgress;

        auto logger = spdlog::get(LogDllID);
        int consecutiveCrashes = 0;

        while (ctx.index < ctx.total && !mCancelRequested) {
            juce::ChildProcess child;
            if (!startChild(child, ctx.index)) {
                for (auto& result : results) {
                    if (result.ok) continue;
                    result.error = "无法启动扫描进程";
                }
                break;
            }

            ctx.gotDone = false;
            ctx.pendingBytes.clear();
            int64_t readPos = 0;
            int64_t lastEventMs = juce::Time::currentTimeMillis();

            Stop stop = Stop::crashed;
            while (!mCancelRequested) {
                readNewBytes(ctx, readPos, lastEventMs);

                if (ctx.gotDone) {
                    stop = Stop::done;
                    break;
                }
                if (!child.isRunning()) {
                    stop = Stop::crashed;
                    break;
                }
                if (juce::Time::currentTimeMillis() - lastEventMs >
                    mOptions.perPluginTimeoutMs) {
                    stop = Stop::timedout;
                    break;
                }

                juce::Thread::sleep(mOptions.pollIntervalMs);
            }

            if (mCancelRequested) stop = Stop::cancelled;

            if (stop == Stop::done) {
                // 子进程写完 DONE 后可能在收尾（释放插件、跑静态析构），给它一点时间
                if (child.isRunning()) {
                    child.waitForProcessToFinish(mOptions.perPluginTimeoutMs);
                }
                if (child.isRunning()) child.kill();
                consecutiveCrashes = 0;
                if (logger) logger->debug("[PluginScan] 批次正常结束，已到 {}", ctx.index);
                continue;
            }

            if (stop == Stop::cancelled) {
                child.kill();
                break;
            }

            // 子进程没了：再读一次进度文件，别漏掉它死前写的最后几行
            readNewBytes(ctx, readPos, lastEventMs);
            if (ctx.gotDone) {
                consecutiveCrashes = 0;
                continue;
            }

            const int failedIndex = ctx.index;
            const juce::String failedFile =
                (failedIndex < ctx.total) ? results[failedIndex].filePath : juce::String{};

            if (stop == Stop::timedout) {
                child.kill();
                markFailed(ctx, failedIndex, "扫描超时（插件卡死）");
                ++ctx.index;
                consecutiveCrashes = 0;
                if (logger)
                    logger->error(
                        "[PluginScan] 超时，已跳过:{}",
                        failedFile.toStdString()
                    );
                continue;
            }

            // Stop::crashed
            markFailed(ctx, failedIndex, "扫描进程崩溃");
            ++ctx.index;
            ++consecutiveCrashes;
            if (logger)
                logger->error(
                    "[PluginScan] 扫描进程崩溃于:{}（连续 {} 次）",
                    failedFile.toStdString(),
                    consecutiveCrashes
                );

            if (consecutiveCrashes >= mOptions.maxConsecutiveCrashes) {
                for (int i = ctx.index; i < ctx.total; ++i) {
                    markFailed(ctx, i, "扫描进程反复崩溃，已放弃剩余文件");
                }
                ctx.index = ctx.total;
                if (logger) logger->error("[PluginScan] 连续崩溃过多，放弃剩余文件");
            }
        }

        // 取消/异常退出时，没轮到的下标统一标成"未扫描"
        for (auto& result : results) {
            if (result.ok || !result.error.empty()) continue;
            result.error = "未扫描";
        }

        return results;
    }

} // namespace PluginHost
