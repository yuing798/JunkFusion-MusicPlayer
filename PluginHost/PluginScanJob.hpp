/**
 * @brief 一次完整的插件扫描任务（后台线程）—— FFI 的 `startPluginScan()` 落到这里
 *
 * ── 它把三个东西串起来 ──────────────────────────────────────────────────
 *
 *     pluginDirs 表
 *          │  (PluginCatalogManage::getExistingDirFiles)
 *          ▼
 *     PathScanner        递归找 .vst3，得到"要扫的模块列表"
 *          │
 *          ▼
 *     PluginScannerProcess   拉起临时子进程扫（崩了/卡死都能续）
 *          │
 *          ▼
 *     PluginCatalogManage::saveFileResult / markFileFailed      ← 入库
 *
 * ── 为什么要单独一个线程 ────────────────────────────────────────────────
 *   扫描本机 12 个插件约 1 秒（规格 §1.5），但用户加了一个装着 300 个插件的
 *   目录就会是几十秒。FFI 是 Flutter 的 UI 线程直接调的，**绝不能阻塞**，
 *   所以这里起一个 juce::Thread，调用方立刻拿到"已开始"。
 *
 * ── 进度只有两态（规格 §5 明确允许）────────────────────────────────────
 *   front-end 轮询 `getPluginScanState()` 拿到
 *   `{state, total, processed, failed, message}`，够画一个"扫描中 5/12"。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     auto& job = *dllManager::getInstance().getPluginScanJob();
 *     if (job.startScan()) { ... 已开始 ... }   // false = 已经在跑
 *     job.toJson();                              // 给 FFI/前端轮询
 */
#pragma once

#include "./PluginCatalogManage.hpp"
#include "./PluginScannerProcess.hpp"

#include <juce_core/juce_core.h>

#include <memory>
#include <string>

namespace PluginHost {

    class PluginScanJob : public juce::Thread {
    public:
        PluginScanJob(PluginCatalogManage& catalog, juce::File scanExe, juce::File workDir);

        /**
         * @brief 开始一次扫描
         * @return false 表示已经在跑了（重复点击直接忽略，不排队）
         */
        bool startScan();

        /** @brief 请求取消并等线程退出（析构/关闭后端时调用） */
        void stopAndWait();

        /** @brief 状态快照，给 FFI 用：{"state":"idle|running|done|failed", ...} */
        std::string toJson() const;

        void run() override;

    private:
        void setPhase(const juce::String& phase);

        /**
         * @brief 标记任务失败
         *
         * 参数是 **UTF-8 字节**（源码里的中文字面量就是 UTF-8，加了 /utf-8 之后
         * 永远是），内部用 juce::String::fromUTF8 转。
         * 不能直接收 juce::String：juce::String 的 `const char*` 构造函数
         * 明确写着"只支持 ASCII，遇到 >127 的字节会断言/乱码"，
         * 实测中文会被一个字节一个字符地拆成乱码。
         */
        void fail(const char* utf8Message);

        PluginCatalogManage& mCatalog;
        juce::File mScanExe;
        juce::File mWorkDir;

        std::unique_ptr<PluginScannerProcess> mScanner;

        mutable juce::CriticalSection mStateLock;
        juce::String mPhase{"idle"};
        juce::String mMessage;
        int mTotal{0};
        int mProcessed{0};
        int mFailed{0};
    };

} // namespace PluginHost
