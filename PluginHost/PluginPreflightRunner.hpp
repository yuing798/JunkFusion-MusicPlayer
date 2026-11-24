/**
 * @brief 预检的**父进程侧**：拉起 `JunkFusionPluginScan.exe --preflight` 并读回报告
 *
 * ── 它解决什么（host-integration.md §7.1 / §6）──────────────────────────
 *   插件实例化 + prepareToPlay + 过一块静音，是**最可能把进程带走**的地方
 *   （实验台结论：崩溃不可捕获，0xC0000005，没有任何回调）。所以在真正把它
 *   放进音频进程之前，先在一个**临时子进程**里走一遍：
 *
 *     子进程正常写出报告      -> 好的，允许插入
 *     子进程没写报告就没了    -> 崩溃，拉黑这个 identifier
 *     子进程卡住超过 timeout  -> 挂死（析构/初始化死锁），也拉黑
 *
 * ── 为什么超时就要杀 ────────────────────────────────────────────────────
 *   "挂死"和"慢"从外面看是一样的，只能靠时间区分。10 秒已经远大于正常插件
 *   （CHOWTapeModel 实测 1.86 秒），再等下去只会让 UI 一直卡着。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     PluginHost::PluginPreflightRunner runner{scanExe};
 *     auto result = runner.run(pluginFile, identifier, 44100.0, 441, 10000);
 *     if (!result.ok) { ...按 result.crashed / timedOut / error 决定拉黑原因... }
 *
 * ── 线程 ────────────────────────────────────────────────────────────────
 *   `run()` 是**阻塞**的（进程启动 + 插件加载 + 超时），只能在非 UI 线程调。
 */
#pragma once

#include <juce_core/juce_core.h>

#include <string>

namespace PluginHost {

    class PluginPreflightRunner {
    public:
        struct Result {
            bool ok{false};

            /// 失败原因（UTF-8，给日志和黑名单用）
            std::string error;

            std::string name;
            int inputs{0};
            int outputs{0};
            int latency{0};
            int params{0};
            bool hasEditor{false};

            /// 子进程没写出报告就没了（= 崩溃）
            bool crashed{false};

            /// 超时被我们杀掉（= 挂死）
            bool timedOut{false};
        };

        explicit PluginPreflightRunner(juce::File scanExe);

        /// 扫描子进程在不在（不在就不做预检，直接放行并记一条警告）
        bool isAvailable() const;

        /**
         * @brief 跑一次预检
         *
         * @param pluginFile 插件模块
         * @param identifier PluginDescription::createIdentifierString()
         * @param sampleRate 设备采样率
         * @param blockSize  设备块大小（延迟值跟块大小有关，用真实值）
         * @param timeoutMs  超时；超了直接 kill，判"挂死"
         */
        Result run(
            const juce::File& pluginFile,
            const std::string& identifier,
            double sampleRate,
            int blockSize,
            int timeoutMs = 10000
        );

    private:
        juce::File mScanExe;
    };

} // namespace PluginHost
