#pragma once

#include "Model/EqFilterMath.hpp"
#include "Model/EqInfo.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

/**
 * @brief 8 段参数均衡器（pro-Q 风格的 DSP 端）
 *
 * ── 设计要点 ──────────────────────────────────────────────────────────
 * 1. **音频线程没有任何运行时多态、没有分配、没有锁**。
 *    控制线程把 EqBandParam 算成 5 个 biquad 系数（b0,b1,b2,a1,a2）后再发布；
 *    音频线程只认系数，不认"这是 Bell 还是 Tile"。系数计算集中在
 *    EqFilterMath（Model/EqFilterMath.hpp），UI 画曲线用的是同一份数学。
 *
 * 2. 每条 band 固定占 **2 个 biquad 段**：
 *      - Tile 需要"低频降 + 高频升"两个 shelf；
 *      - LowPass/HighPass 的 24 dB/oct 需要两段级联；
 *      - 用不上的段写直通系数（b0=1，其余 0），所以换类型时永远是"系数渐变"，
 *        不会出现段数变化导致的爆音。
 *
 * 3. 发布用 seqlock：控制线程改 mPending 前后各动一次 mVersion，
 *    音频线程在块首校验版本号，校验不过就这个块继续用旧系数。
 *    **绝不能把系数撕裂**（a1 被读成两半会让滤波器自激）。
 *
 * 4. 系数与干湿比每 kCoeffUpdateInterval(64) 个样本做一次一阶平滑（时间常数 20ms）。
 *    逐样本平滑的话 8 段 × 每样本 5 个系数 太浪费，而 64 样本 ≈ 1.3ms，听不出台阶。
 *
 * ── 线程模型 ─────────────────────────────────────────────────────────
 *   prepare/reset :消息线程（prepareToPlay 不会再与 process 并发）
 *   setXxx        :OSC 接收线程 / ZMQ 拉取线程（内部 mControlMutex + seqlock）
 *   process       :音频线程
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     mEq.prepare(sampleRate, maxBlockSize, numOutputChannels);
 *     mEq.setAll(param, rev);   // 或 setBand / setBandLive / setMaster
 *     mEq.process(buffer, buffer.getNumSamples());
 */
class Eq {
public:
    using Coefficients = EqFilterMath::Coefficients;

    static constexpr int kMaxBands = EqBandParam::kMaxBands;     // 8
    static constexpr int kMaxSections = EqFilterMath::kMaxSections; // 2
    static constexpr int kMaxChannels = 16;                    // 编译期上限；CPU 只按实际通道数走
    static constexpr int kCoeffUpdateInterval = 64;             // 系数更新粒度（样本）
    static constexpr double kSmoothTimeSec = 0.02;              // 参数平滑时间常数

    // ── 生命周期（消息线程） ────────────────────────────────────────
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    // ── 控制面（OSC / ZMQ 拉取线程；rev == 0 表示无版本信息，无条件应用）
    void setBand(const EqBandParam& band, uint32_t rev);
    void setBandLive(int idx, float freqHz, float gainDb, float q, float shelfSlope, uint32_t rev);
    void setMaster(bool enabled, float masterGainDb, uint32_t rev);
    void setAll(const EqAllParam& all, uint32_t rev);

    // ── 音频线程 ────────────────────────────────────────────────────
    void process(juce::AudioBuffer<float>& buffer, int numSamples);

private:
    struct BiquadState {
        float z1{0.0f}, z2{0.0f};
    };
    /** @brief 一次发布出去的完整快照（系数 + 干湿比 + 主增益） */
    struct Snapshot {
        std::array<std::array<Coefficients, kMaxSections>, kMaxBands> bands{};
        std::array<float, kMaxBands> bandMix{};
        float masterGain{1.0f};
        float masterMix{1.0f};
    };

    // ── 控制面内部（全部要求已持有 mControlMutex） ──────────────────
    bool acceptRev(uint32_t rev);
    void beginPublish() noexcept;
    void endPublish() noexcept;
    void rebuildBandLocked(int idx);
    void rebuildAllLocked();
    void rebuildMasterLocked();

    // ── 音频面内部 ──────────────────────────────────────────────────
    void loadSnapshot() noexcept;
    void advanceSmoothing() noexcept;

    // 控制面状态
    EqAllParam mParams;
    double mSampleRate{48000.0};
    int mNumChannels{0};
    bool mPrepared{false};
    float mSmoothCoeff{0.0645f};
    uint32_t mLastRev{0};
    std::mutex mControlMutex;

    // 交界：mPending 由控制线程写、音频线程只读（seqlock 保护）
    Snapshot mPending{};
    std::atomic<uint32_t> mVersion{0};

    // 音频面
    Snapshot mActive{};  // 由 mPending 快照出来的目标值
    Snapshot mCurrent{}; // 正在平滑逼近 mActive 的当前值
    std::array<std::array<std::array<BiquadState, kMaxSections>, kMaxBands>, kMaxChannels> mState{};
};
