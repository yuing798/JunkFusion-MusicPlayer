#include "./PluginScanJob.hpp"

#include "./PluginCategory.hpp"
#include "./PluginPathScanner.hpp"

#include "Utils/Yvar.hpp"
#include "Utils/constants.h"

#include <spdlog/spdlog.h>

namespace PluginHost {

    PluginScanJob::PluginScanJob(
        PluginCatalogManage& catalog,
        juce::File scanExe,
        juce::File workDir
    )
        : juce::Thread("PluginScanJob"), mCatalog(catalog), mScanExe(std::move(scanExe)),
          mWorkDir(std::move(workDir)) {}

    void PluginScanJob::setPhase(const juce::String& phase) {
        const juce::ScopedLock lock{mStateLock};
        mPhase = phase;
    }

    void PluginScanJob::fail(const char* utf8Message) {
        const juce::ScopedLock lock{mStateLock};
        mPhase = "failed";
        // juce::String 的 const char* 构造函数只吃 ASCII（>127 的字节会被拆成
        // 单字节字符，中文直接乱码），中文必须走 fromUTF8。
        mMessage = juce::String::fromUTF8(utf8Message);
    }

    bool PluginScanJob::startScan() {
        {
            const juce::ScopedLock lock{mStateLock};
            if (mPhase == "running") return false;
            mPhase = "running";
            mMessage = "";
            mTotal = 0;
            mProcessed = 0;
            mFailed = 0;
        }

        if (isThreadRunning()) {
            // 上一轮刚结束但线程还没完全回收：等它退干净再启
            stopThread(3000);
        }

        startThread();
        return true;
    }

    void PluginScanJob::stopAndWait() {
        // mScanner 是工作线程创建/销毁的，这里必须拿着锁读，
        // 否则和控制线程上的 make_unique/reset 撞车（unique_ptr 的读不是原子的）
        {
            const juce::ScopedLock lock{mStateLock};
            if (mScanner != nullptr) mScanner->requestCancel();
        }
        signalThreadShouldExit();
        stopThread(5000);
    }

    void PluginScanJob::run() {
        auto logger = spdlog::get(LogDllID);
        if (logger) logger->info("[PluginScan] 开始扫描");

        if (!mScanExe.existsAsFile()) {
            if (logger)
                logger->error(
                    "[PluginScan] 扫描进程不存在:{}",
                    mScanExe.getFullPathName().toStdString()
                );
            fail("扫描进程不存在，请检查安装是否完整");
            return;
        }

        // ── 1. 目录 → 模块列表 ──
        const auto dirs = mCatalog.getExistingDirFiles();
        if (dirs.empty()) {
            fail("没有任何可用的插件目录");
            return;
        }

        PathScanner pathScanner;
        const auto found = pathScanner.scanAll(dirs);
        const auto& modules = found.modules;

        if (logger)
            logger->info(
                "[PluginScan] 目录 {} 个，遍历 {} 项，找到模块 {} 个",
                static_cast<int>(dirs.size()),
                found.filesVisited,
                static_cast<int>(modules.size())
            );

        {
            const juce::ScopedLock lock{mStateLock};
            mTotal = static_cast<int>(modules.size());
        }

        if (modules.empty()) {
            const juce::ScopedLock lock{mStateLock};
            mPhase = "done";
            mMessage = juce::String::fromUTF8("没有找到任何 .vst3");
            return;
        }

        // ── 2. 子进程批量扫描 ──
        PluginScannerProcess::Options options;
        options.scanExe = mScanExe;
        options.workDir = mWorkDir;
        {
            const juce::ScopedLock lock{mStateLock};
            mScanner = std::make_unique<PluginScannerProcess>(options);
        }

        auto results = mScanner->scan(modules, [this](int processed, int total, const juce::String&) {
            const juce::ScopedLock lock{mStateLock};
            mTotal = total;
            mProcessed = processed;
        });

        {
            const juce::ScopedLock lock{mStateLock};
            mScanner.reset();
        }

        if (threadShouldExit()) {
            setPhase("idle");
            return;
        }

        // ── 3. 入库 ──
        const auto now = juce::Time::currentTimeMillis();
        int okCount = 0;
        int failedCount = 0;

        for (const auto& result : results) {
            if (threadShouldExit()) break;

            const auto filePath = result.filePath.toStdString();

            if (result.ok) {
                mCatalog.saveFileResult(filePath, result.records, now);
                ++okCount;

                if (logger) {
                    for (const auto& record : result.records) {
                        // 规格 §2 建议：把命中的规则写进日志，
                        // 分类出问题时能一眼看出是元数据缺失还是通道数异常
                        logger->debug(
                            "[PluginScan] {} → {}",
                            record.meta.name,
                            describeDecision(record.meta, {record.type, record.rule})
                        );
                    }
                }
            } else {
                mCatalog.markFileFailed(filePath, result.error, now);
                ++failedCount;
                if (logger)
                    logger->warn(
                        "[PluginScan] 失败 {}: {}",
                        filePath,
                        result.error
                    );
            }
        }

        {
            const juce::ScopedLock lock{mStateLock};
            mPhase = "done";
            mProcessed = static_cast<int>(results.size());
            mFailed = failedCount;
            // 中文摘要用 std::string 拼再接 fromUTF8：这样整句只有一处编码转换，
            // 不会出现"前半句乱码、后半句正常"（juce::String 混拼 const char* 的坑）
            std::string summary = "扫描完成：成功 " + std::to_string(okCount) + " 个模块，失败 " +
                                  std::to_string(failedCount) + " 个";
            mMessage = juce::String::fromUTF8(summary.c_str());
        }

        if (logger)
            logger->info(
                "[PluginScan] 结束：成功 {} / 失败 {}",
                okCount,
                failedCount
            );
    }

    std::string PluginScanJob::toJson() const {
        Yvar obj = Yvar::object();
        {
            const juce::ScopedLock lock{mStateLock};
            obj.set("state", mPhase.toStdString());
            obj.set("total", mTotal);
            obj.set("processed", mProcessed);
            obj.set("failed", mFailed);
            obj.set("message", mMessage.toStdString());
        }
        return obj.dump();
    }

} // namespace PluginHost
