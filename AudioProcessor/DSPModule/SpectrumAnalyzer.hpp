/**
 * @brief 实时频谱分析（音频进程侧）
 *
 * ── 数据流 ────────────────────────────────────────────────────────────
 *   音频回调线程  ──pushSamples()──>  单声道环形缓冲（无锁，只 memcpy）
 *                                            │
 *   本类的分析线程（juce::Thread）<──────────┘  每 20ms 取最后 2048 个样本
 *        │  加 Hann 窗 → FFT → 对数分频（96 段）→ 攻击/释放平滑
 *        ▼
 *   共享内存（seqlock）──> Coordinator ──FFI──> 前端
 *
 * ── 为什么不在音频线程做 FFT ─────────────────────────────────────────
 * 2048 点 FFT 大概几十微秒，放在回调里不是不能跑，但播放器要保证
 * 任何情况下都不卡，所以音频线程只做一次降混 + 写入环形缓冲（<2µs）。
 *
 * ── 开关 ──────────────────────────────────────────────────────────────
 * 默认关闭。播放页打开时由 Coordinator 发消息打开，关页面时关掉，
 * 避免没人在看的时候空转 FFT。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     mSpectrum.prepare(sampleRate);                 // prepareToPlay
 *     mSpectrum.pushSamples(buffer);                 // processBlock 末尾
 *     mSpectrum.attachSharedMemory(name, bytes);     // 收到 Coordinator 消息
 *     mSpectrum.setEnabled(true);                    // 页面打开
 */
#pragma once

#include "Utils/sharedMemory.hpp"
#include "Utils/spectrumShared.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class SpectrumAnalyzer : private juce::Thread {
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override;

    /// 采样率变了就重建频段边界（只改本类状态，不动共享内存）
    void prepare(double sampleRate);

    /// 音频线程调用：只做降混 + 写环形缓冲，无锁无分配
    void pushSamples(const juce::AudioBuffer<float>& buffer) noexcept;

    /**
     * @brief 接上 Coordinator 创建的共享内存
     *
     * @param name 完整名字（带平台前缀，由 Coordinator 发过来）
     * @param bytes 字节数，必须等于 sizeof(SpectrumShm::Frame)
     */
    bool attachSharedMemory(const std::string& name, size_t bytes);

    void setEnabled(bool enabled) noexcept;
    bool isEnabled() const noexcept { return mEnabled.load(std::memory_order_relaxed); }

private:
    void run() override;

    /// 取一帧做 FFT 并写进共享内存
    void analyseOnce(SpectrumShm::Frame& frame);

    /// 原地基 2 FFT（radix-2，要求长度是 2 的幂）
    static void fftRadix2(std::vector<float>& re, std::vector<float>& im);

    /// 按当前采样率重算 96 段的频谱边界（FFT bin 下标）
    void rebuildBandEdges();

    static constexpr int kFftOrder = 11;              // 2048 点
    static constexpr int kFftSize = 1 << kFftOrder;   // = 2048
    static constexpr int kHalfSize = kFftSize / 2;    // 可用 bin 数
    static constexpr uint32_t kRingSize = 1u << 14;   // 16384 个样本
    static constexpr int kPeriodMs = 20;              // 分析线程节奏 ≈ 50Hz

    // ── 环形缓冲（单生产者单消费者） ──
    std::array<float, kRingSize> mRing{};
    std::atomic<uint32_t> mWriteIndex{0};
    std::atomic<uint32_t> mReadIndex{0};

    std::atomic<double> mSampleRate{48000.0};
    std::atomic<bool> mEnabled{false};

    /// prepare() 只置脏，真正的重建在分析线程里做，避免和读 mBandEdges 撞车
    std::atomic<bool> mBandEdgesDirty{false};

    // ── 共享内存 ──
    // attach 时换绑，分析线程用原子指针读；旧的映射不立刻释放，
    // 全部留在 mAttachments 里（attach 一生只发生一两次，不值得为它写回收）
    std::atomic<SpectrumShm::Frame*> mFrame{nullptr};
    std::vector<std::unique_ptr<SharedMemory>> mAttachments;
    std::mutex mAttachMutex;

    // ── FFT 工作区 ──
    std::vector<float> mWindow;    // Hann 窗
    std::vector<float> mRe;
    std::vector<float> mIm;
    std::vector<int> mBandEdges;   // kBandCount + 1 个边界

    // ── 平滑状态 ──
    std::array<float, SpectrumShm::kBandCount> mBands{};
    std::array<float, SpectrumShm::kBandCount> mPeaks{};
};
