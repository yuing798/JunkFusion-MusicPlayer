/**
 * @brief 批量扫描的**父进程侧**：拉起/监管 `JunkFusionPluginScan.exe`，断点续扫
 *
 * ── 它解决的核心问题（规格 §3.3）────────────────────────────────────────
 * 扫描进程会被坏插件带走。所以这里的循环不是"启动一次、读完结果"，而是：
 *
 *     从 index i 开始启动子进程
 *        ├─ 收到 END/FAIL → index 前进，继续等同一个子进程
 *        ├─ 子进程没了（崩溃）→ 当前 index 记 failed，从 index+1 重新启动
 *        └─ 15 秒没有任何新事件（卡死）→ 杀掉，当前 index 记 failed，从 index+1 重启
 *
 * 几个刻意的取舍：
 *   1. **不是"一个插件一个进程"**：进程启动 + DLL 加载 是主要成本，
 *      12 个插件开 12 个进程会让扫描从 1 秒变成十几秒（规格 §3.3 明确禁止）。
 *   2. **不读 stdout 管道**：第三方插件往 stdout 乱打会填满管道缓冲把子进程卡死。
 *      结果走 --progress 文件，这里按字节轮询追加内容（见 readNewBytes）。
 *   3. **崩溃次数有上限**：连续崩 5 次说明这台机器上扫描进程根本跑不起来
 *      （缺 VC 运行库、杀软拦截…），继续重启只会无限循环，剩下的直接标失败。
 *
 * ── 线程模型 ────────────────────────────────────────────────────────────
 *   `scan()` 是**阻塞**的，必须在后台线程上跑（PluginScanJob 负责）。
 *   本类自己不建线程，也不碰数据库——结果交回上层入库。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     PluginScannerProcess::Options opt;
 *     opt.scanExe = exeDir.getChildFile("JunkFusionPluginScan.exe");
 *     opt.workDir = cacheDir.getChildFile("pluginScan");
 *     PluginScannerProcess scanner{opt};
 *     auto results = scanner.scan(modules, [](int done, int total, auto& file) { ... });
 */
#pragma once

#include "./PluginScanProtocol.hpp"
#include "./PluginTypes.hpp"

#include <juce_core/juce_core.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PluginHost {

    class PluginScannerProcess {
    public:
        /** @brief 一个模块文件的扫描结果（对应列表里的一个 index） */
        struct FileResult {
            juce::String filePath;
            std::vector<Record> records;
            bool ok{false};
            std::string error;
        };

        struct Options {
            juce::File scanExe;                        // JunkFusionPluginScan.exe
            juce::File workDir;                        // <cacheDir>/pluginScan
            int perPluginTimeoutMs{15000};             // 规格 §3.3 建议 15s
            int pollIntervalMs{50};                    // 轮询进度文件的间隔
            int maxConsecutiveCrashes{5};              // 连续崩溃上限
        };

        using ProgressFn =
            std::function<void(int processed, int total, const juce::String& currentFile)>;

        explicit PluginScannerProcess(Options options);

        /** @brief 阻塞扫描。取消后已扫到的结果仍然返回（不会丢） */
        std::vector<FileResult> scan(
            const std::vector<juce::File>& modules,
            const ProgressFn& onProgress
        );

        /** @brief 从别的线程请求取消（下一个轮询周期生效） */
        void requestCancel() noexcept { mCancelRequested = true; }

        /** @brief 扫描器可执行文件在不在（不在就不用白跑一趟） */
        bool isScanExePresent() const { return mOptions.scanExe.existsAsFile(); }

        juce::String getScanExePath() const { return mOptions.scanExe.getFullPathName(); }

    private:
        /** @brief 一轮子进程运行期间的共享状态 */
        struct RunContext {
            std::vector<FileResult>* results{nullptr};
            int total{0};
            int index{0};              // 第一个还没结束的下标
            bool gotDone{false};       // 收到 DONE
            std::string pendingBytes;  // 还没凑成整行的残留字节（按字节存，避免 UTF-8 被劈开）
            const ProgressFn* onProgress{nullptr};
        };

        /** @brief 子进程这一轮是怎么结束的 */
        enum class Stop {
            done,      // 正常跑完（DONE）
            crashed,   // 进程没了但没到 DONE
            timedout,  // 15s 没有任何新事件
            cancelled  // 上层要求取消
        };

        bool writeFileList(const std::vector<juce::File>& modules);
        bool startChild(juce::ChildProcess& child, int startIndex);

        /** @brief 把进度文件里新增的字节读进来，切行、分发事件 */
        void readNewBytes(RunContext& ctx, int64_t& readPos, int64_t& lastEventMs);

        /** @brief 处理一整行（只认 JFPLUG 前缀） */
        void handleLine(
            RunContext& ctx,
            const std::string& line,
            int64_t& lastEventMs
        );

        /** @brief 标记某个下标失败并记录原因 */
        void markFailed(RunContext& ctx, int index, const std::string& reason);

        Options mOptions;
        juce::File mListFile;
        juce::File mProgressFile;
        std::atomic<bool> mCancelRequested{false};
    };

} // namespace PluginHost
