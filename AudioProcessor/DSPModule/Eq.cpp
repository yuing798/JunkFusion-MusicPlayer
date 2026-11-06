#include "./Eq.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

// ── 生命周期 ────────────────────────────────────────────────────────────
void Eq::prepare(double sampleRate, int maxBlockSize, int numChannels) {
    juce::ignoreUnused(maxBlockSize);
    std::lock_guard<std::mutex> lock(mControlMutex);

    mSampleRate = (sampleRate > 0.0) ? sampleRate : 48000.0;
    mNumChannels = juce::jlimit(0, kMaxChannels, numChannels);

    // 一阶平滑系数：每 kCoeffUpdateInterval 个样本走掉剩余误差的一个固定比例
    const double intervalSec = static_cast<double>(kCoeffUpdateInterval) / mSampleRate;
    mSmoothCoeff = static_cast<float>(1.0 - std::exp(-intervalSec / kSmoothTimeSec));
    mSmoothCoeff = juce::jlimit(0.0f, 1.0f, mSmoothCoeff);

    rebuildAllLocked();

    // prepare 不在实时线程，直接对齐，避免启动/换设备时听到一段渐变
    mActive = mPending;
    mCurrent = mPending;
    mPrepared = true;

    for (auto& channel : mState)
        for (auto& band : channel)
            for (auto& section : band) section = BiquadState{};
}

void Eq::reset() {
    for (auto& channel : mState)
        for (auto& band : channel)
            for (auto& section : band) section = BiquadState{};

    mCurrent = mActive; // 清完状态后不要再来一段渐变
}

// ── 控制面 ──────────────────────────────────────────────────────────────
bool Eq::acceptRev(uint32_t rev) {
    if (rev == 0u) return true;        // 无版本信息，无条件应用
    if (rev <= mLastRev) return false; // 迟到的旧帧，丢弃
    mLastRev = rev;
    return true;
}

void Eq::beginPublish() noexcept {
    mVersion.fetch_add(1, std::memory_order_relaxed); // 变奇数：正在写
}

void Eq::endPublish() noexcept {
    mVersion.fetch_add(1, std::memory_order_release); // 变偶数：写完了
}

void Eq::rebuildBandLocked(int idx) {
    const size_t index = static_cast<size_t>(idx);
    const auto& band = mParams.bands[index];

    beginPublish();
    EqFilterMath::buildBand(band, mSampleRate, mPending.bands[index]);
    mPending.bandMix[index] = band.enabled ? 1.0f : 0.0f;
    endPublish();
}

void Eq::rebuildMasterLocked() {
    beginPublish();
    mPending.masterGain = EqFilterMath::dbToLinear(mParams.masterGainDb);
    mPending.masterMix = mParams.enabled ? 1.0f : 0.0f;
    endPublish();
}

void Eq::rebuildAllLocked() {
    beginPublish();
    for (int i = 0; i < kMaxBands; ++i) {
        const size_t index = static_cast<size_t>(i);
        EqFilterMath::buildBand(mParams.bands[index], mSampleRate, mPending.bands[index]);
        mPending.bandMix[index] = mParams.bands[index].enabled ? 1.0f : 0.0f;
    }
    mPending.masterGain = EqFilterMath::dbToLinear(mParams.masterGainDb);
    mPending.masterMix = mParams.enabled ? 1.0f : 0.0f;
    endPublish();
}

void Eq::setBand(const EqBandParam& band, uint32_t rev) {
    std::lock_guard<std::mutex> lock(mControlMutex);
    if (!acceptRev(rev)) return;

    EqBandParam param = band;
    param.idx = juce::jlimit(0, kMaxBands - 1, band.idx);
    mParams.bands[static_cast<size_t>(param.idx)] = param;
    rebuildBandLocked(param.idx);
}

void Eq::setBandLive(int idx, float freqHz, float gainDb, float q, float shelfSlope, uint32_t rev) {
    std::lock_guard<std::mutex> lock(mControlMutex);
    if (!acceptRev(rev)) return;

    const int safeIdx = juce::jlimit(0, kMaxBands - 1, idx);
    auto& band = mParams.bands[static_cast<size_t>(safeIdx)];

    // 拖拽只改连续量：类型/开关由结构报文负责，这里绝不能把它们改掉
    band.freqHz = freqHz;
    band.gainDb = gainDb;
    band.q = q;
    band.shelfSlope = shelfSlope;
    rebuildBandLocked(safeIdx);
}

void Eq::setMaster(bool enabled, float masterGainDb, uint32_t rev) {
    std::lock_guard<std::mutex> lock(mControlMutex);
    if (!acceptRev(rev)) return;

    mParams.enabled = enabled;
    mParams.masterGainDb = masterGainDb;
    rebuildMasterLocked();
}

void Eq::setAll(const EqAllParam& all, uint32_t rev) {
    std::lock_guard<std::mutex> lock(mControlMutex);
    if (!acceptRev(rev)) return;

    mParams = all;
    for (int i = 0; i < kMaxBands; ++i) mParams.bands[static_cast<size_t>(i)].idx = i;
    rebuildAllLocked();
}

// ── 音频面 ──────────────────────────────────────────────────────────────
void Eq::loadSnapshot() noexcept {
    // seqlock：版本号奇数说明控制线程正在写，这个块继续用旧系数
    const uint32_t versionBefore = mVersion.load(std::memory_order_acquire);
    if ((versionBefore & 1u) != 0u) return;

    Snapshot temp = mPending; // 拷贝期间若被改写，下面的版本校验会把它判废
    std::atomic_thread_fence(std::memory_order_acquire);

    if (mVersion.load(std::memory_order_relaxed) != versionBefore) return;
    mActive = temp;
}

void Eq::advanceSmoothing() noexcept {
    const float a = mSmoothCoeff;

    for (int b = 0; b < kMaxBands; ++b) {
        for (int s = 0; s < kMaxSections; ++s) {
            auto& current = mCurrent.bands[static_cast<size_t>(b)][static_cast<size_t>(s)];
            const auto& target = mActive.bands[static_cast<size_t>(b)][static_cast<size_t>(s)];

            current.b0 += (target.b0 - current.b0) * a;
            current.b1 += (target.b1 - current.b1) * a;
            current.b2 += (target.b2 - current.b2) * a;
            current.a1 += (target.a1 - current.a1) * a;
            current.a2 += (target.a2 - current.a2) * a;
        }

        auto& mix = mCurrent.bandMix[static_cast<size_t>(b)];
        const float targetMix = mActive.bandMix[static_cast<size_t>(b)];
        mix += (targetMix - mix) * a;
        if (targetMix <= 0.0f && mix < 1.0e-4f) mix = 0.0f; // 收敛到 0，让 process 能整段跳过
    }

    mCurrent.masterGain += (mActive.masterGain - mCurrent.masterGain) * a;
    mCurrent.masterMix += (mActive.masterMix - mCurrent.masterMix) * a;
    if (mActive.masterMix <= 0.0f && mCurrent.masterMix < 1.0e-4f) mCurrent.masterMix = 0.0f;
}

void Eq::process(juce::AudioBuffer<float>& buffer, int numSamples) {
    juce::ScopedNoDenormals noDenormals; // biquad 静音时状态会下沉到 denormal，性能会掉一个量级

    if (!mPrepared || numSamples <= 0) return;

    const int numChannels = juce::jmin(buffer.getNumChannels(), mNumChannels);
    if (numChannels <= 0) return;

    loadSnapshot();

    int done = 0;
    while (done < numSamples) {
        const int blockLen = juce::jmin(kCoeffUpdateInterval, numSamples - done);
        advanceSmoothing(); // 每 64 个样本更新一次系数/干湿比，避免逐样本的开销

        for (int ch = 0; ch < numChannels; ++ch) {
            float* samples = buffer.getWritePointer(ch) + done;

            for (int b = 0; b < kMaxBands; ++b) {
                const float mix = mCurrent.bandMix[static_cast<size_t>(b)];
                if (mix <= 0.0f) continue; // 关掉的 band 整段跳过

                const auto& section0 = mCurrent.bands[static_cast<size_t>(b)][0];
                const auto& section1 = mCurrent.bands[static_cast<size_t>(b)][1];
                auto& state0 = mState[static_cast<size_t>(ch)][static_cast<size_t>(b)][0];
                auto& state1 = mState[static_cast<size_t>(ch)][static_cast<size_t>(b)][1];

                for (int i = 0; i < blockLen; ++i) {
                    const float dry = samples[i];
                    float wet = dry;

                    // 段 0（Transposed Direct Form II）
                    float y = section0.b0 * wet + state0.z1;
                    state0.z1 = section0.b1 * wet - section0.a1 * y + state0.z2;
                    state0.z2 = section0.b2 * wet - section0.a2 * y;
                    wet = y;

                    // 段 1（不需要时是直通系数）
                    y = section1.b0 * wet + state1.z1;
                    state1.z1 = section1.b1 * wet - section1.a1 * y + state1.z2;
                    state1.z2 = section1.b2 * wet - section1.a2 * y;
                    wet = y;

                    // 干湿混合：关掉/打开 band 都靠它淡入淡出，不会爆音
                    samples[i] = dry + (wet - dry) * mix;
                }
            }

            // 主增益 + 整机旁通
            const float masterMix = mCurrent.masterMix;
            if (masterMix > 0.0f) {
                const float gain = mCurrent.masterGain;
                for (int i = 0; i < blockLen; ++i) {
                    const float dry = samples[i];
                    samples[i] = dry + (dry * gain - dry) * masterMix;
                }
            }
        }

        done += blockLen;
    }
}
