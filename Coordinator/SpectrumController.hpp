/**
 * @brief 频谱的 Coordinator 侧（共享内存的创建者 + 前端读取入口）
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * 前端不能直接跟音频进程说话，所以由 Coordinator 出面：
 *   1. 创建命名共享内存（Utils/sharedMemory.hpp）；
 *   2. 把"名字 + 字节数"通过 ZMQ 发给音频进程（音频进程打开同一块）；
 *   3. 前端通过 FFI 调 getBands()/getPeaks()，这里用 seqlock 读一帧快照。
 *
 * ── 为什么数据不经过 Coordinator 转发 ─────────────────────────────────
 * 频谱是 50Hz 的连续流，走 ZMQ 要多一次序列化 + 线程切换，延迟和抖动都不划算。
 * 数据面直连共享内存，Coordinator 只做"递名片"和"读快照"。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     mSpectrumController->init(sendLambda);   // dllManager::init 里一次
 *     mSpectrumController->setEnabled(true);   // 播放页打开
 *     const float* bands = mSpectrumController->getBands();  // 每帧读
 *
 * ⚠️ getBands()/getPeaks() 返回的是内部缓冲指针，下次调用时会被覆盖，
 *    调用方（FFI → Dart）必须立刻拷走。
 */
#pragma once

#include "Utils/sharedMemory.hpp"
#include "Utils/spectrumShared.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class SpectrumController {
public:
    /**
     * @brief 创建共享内存并通知音频进程
     *
     * @param send 把 JSON 字符串发给音频进程的回调（dllManager::sendMessage2AudioProcess）
     */
    void init(const std::function<void(const std::string&)>& send);

    /// 开/关音频进程的频谱分析（播放页进出时调）
    void setEnabled(bool enabled);

    /// 当前频段值（0~1），没有数据时返回 nullptr
    const float* getBands();

    /// 峰值保持值（0~1），没有数据时返回 nullptr
    const float* getPeaks();

    /// 频段数量（固定 96，前端用它分配数组）
    static constexpr int bandCount() { return static_cast<int>(SpectrumShm::kBandCount); }

private:
    /// 用 seqlock 读一帧到内部缓冲；成功返回 true
    bool refresh();

    /// 当前进程号（拼进共享内存名字，避免多开时撞车）
    static uint32_t currentProcessId() noexcept;

    std::unique_ptr<SharedMemory> mShm;
    SpectrumShm::Frame* mFrame{nullptr};

    /// 发给音频进程的通道（init 时注入）
    std::function<void(const std::string&)> mSend;

    std::array<float, SpectrumShm::kBandCount> mBands{};
    std::array<float, SpectrumShm::kBandCount> mPeaks{};
};
