#include "./SpectrumAnalyzer.hpp"

#include "Utils/constants.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>

namespace {
// 平滑与归一化的几个可调参数（都是"看上去舒服"的值，不是物理量）
constexpr float kAttack = 0.55f;      // 上升沿快一点，跳起来有冲击力
constexpr float kRelease = 0.12f;     // 下降沿慢一点，看起来不闪
constexpr float kPeakDecay = 0.015f;  // 峰值每秒衰减 ≈ 0.75
constexpr float kDbFloor = -75.0f;    // 低于这个算 0
constexpr float kDbCeil = -20.0f;     // 高于这个算 1
} // namespace

SpectrumAnalyzer::SpectrumAnalyzer() : juce::Thread("SpectrumAnalyzer") {
    mWindow.resize(kFftSize);
    for (int i = 0; i < kFftSize; ++i) {
        // Hann 窗：两端压低，减少频谱泄漏
        mWindow[i] =
            0.5f - 0.5f * std::cos(2.0f * juce::MathConstants<float>::pi * (float)i / (float)(kFftSize - 1));
    }
    mRe.assign(kFftSize, 0.0f);
    mIm.assign(kFftSize, 0.0f);
    mBandEdges.assign(SpectrumShm::kBandCount + 1, 0);

    rebuildBandEdges();
    startThread(juce::Thread::Priority::low);
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
    stopThread(2000);
}

void SpectrumAnalyzer::prepare(double sampleRate) {
    if (sampleRate <= 0.0) return;
    mSampleRate.store(sampleRate, std::memory_order_relaxed);
    mBandEdgesDirty.store(true, std::memory_order_release);
}

void SpectrumAnalyzer::rebuildBandEdges() {
    const double sampleRate = mSampleRate.load(std::memory_order_relaxed);
    if (sampleRate <= 0.0) return;

    const double maxHz = std::min(SpectrumShm::kMaxHz, sampleRate * 0.5);
    const double minHz = SpectrumShm::kMinHz;

    // 对数分频：第 i 段的边界频率
    for (int i = 0; i <= (int)SpectrumShm::kBandCount; ++i) {
        const double t = (double)i / (double)SpectrumShm::kBandCount;
        const double hz = minHz * std::pow(maxHz / minHz, t);
        int bin = (int)std::lround(hz * (double)kFftSize / sampleRate);
        bin = std::clamp(bin, 1, kHalfSize - 1);
        mBandEdges[(size_t)i] = bin;
    }
    // 保证每个 band 至少占一个 bin，否则高频会挤成 0 宽度
    for (int i = 1; i <= (int)SpectrumShm::kBandCount; ++i) {
        if (mBandEdges[(size_t)i] <= mBandEdges[(size_t)i - 1]) {
            mBandEdges[(size_t)i] = std::min(mBandEdges[(size_t)i - 1] + 1, kHalfSize - 1);
        }
    }
}

void SpectrumAnalyzer::pushSamples(const juce::AudioBuffer<float>& buffer) noexcept {
    if (!mEnabled.load(std::memory_order_relaxed)) return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0) return;

    const uint32_t write = mWriteIndex.load(std::memory_order_relaxed);
    const uint32_t read = mReadIndex.load(std::memory_order_acquire);

    // 缓冲满了就丢这一块（分析线程 20ms 采一次，正常情况永远追得上）
    if ((uint32_t)numSamples > kRingSize - (write - read)) return;

    for (int i = 0; i < numSamples; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) sum += buffer.getSample(ch, i);
        mRing[(write + (uint32_t)i) & (kRingSize - 1u)] = sum / (float)numChannels;
    }

    mWriteIndex.store(write + (uint32_t)numSamples, std::memory_order_release);
}

bool SpectrumAnalyzer::attachSharedMemory(const std::string& name, size_t bytes) {
    if (name.empty()) return false;
    if (bytes < SpectrumShm::kByteSize) return false;

    std::lock_guard<std::mutex> lock(mAttachMutex);

    auto shared = std::make_unique<SharedMemory>();
    if (!shared->open(name, bytes)) {
        if (auto logger = spdlog::get(LogAudioID))
            logger->error("SpectrumAnalyzer::attachSharedMemory 打开共享内存失败:{}", name);
        return false;
    }

    auto* frame = static_cast<SpectrumShm::Frame*>(shared->data());
    if (!SpectrumShm::isValidFrame(*frame)) {
        if (auto logger = spdlog::get(LogAudioID))
            logger->error("SpectrumAnalyzer::attachSharedMemory 数据头无效（magic/version 不符）");
        return false;
    }

    mAttachments.push_back(std::move(shared));
    mFrame.store(frame, std::memory_order_release);

    if (auto logger = spdlog::get(LogAudioID)) logger->debug("频谱共享内存已接上:{}", name);
    return true;
}

void SpectrumAnalyzer::setEnabled(bool enabled) noexcept {
    mEnabled.store(enabled, std::memory_order_relaxed);

    // 关闭时把共享内存里的 enabled 归零，前端据此知道"没数据了"
    if (!enabled) {
        if (auto* frame = mFrame.load(std::memory_order_acquire)) {
            SpectrumShm::writeBegin(*frame);
            frame->header.enabled = 0;
            SpectrumShm::writeEnd(*frame);
        }
    }
}

void SpectrumAnalyzer::run() {
    while (!threadShouldExit()) {
        if (mBandEdgesDirty.exchange(false, std::memory_order_acq_rel)) {
            rebuildBandEdges();
        }

        auto* frame = mFrame.load(std::memory_order_acquire);
        if (frame != nullptr && mEnabled.load(std::memory_order_relaxed)) {
            analyseOnce(*frame);
        }
        wait(kPeriodMs);
    }
}

void SpectrumAnalyzer::analyseOnce(SpectrumShm::Frame& frame) {
    const uint32_t write = mWriteIndex.load(std::memory_order_acquire);
    const uint32_t read = mReadIndex.load(std::memory_order_relaxed);
    const uint32_t available = write - read;

    if (available < (uint32_t)kFftSize) return; // 还没攒够一帧

    const uint32_t start = write - (uint32_t)kFftSize;
    for (int i = 0; i < kFftSize; ++i) {
        const float sample = mRing[(start + (uint32_t)i) & (kRingSize - 1u)];
        mRe[(size_t)i] = sample * mWindow[(size_t)i];
        mIm[(size_t)i] = 0.0f;
    }
    mReadIndex.store(write, std::memory_order_release);

    fftRadix2(mRe, mIm);

    const float sampleRate = (float)mSampleRate.load(std::memory_order_relaxed);

    for (uint32_t band = 0; band < SpectrumShm::kBandCount; ++band) {
        const int begin = mBandEdges[band];
        const int end = mBandEdges[band + 1];

        float maxMagnitude = 0.0f;
        for (int bin = begin; bin < end; ++bin) {
            const float re = mRe[(size_t)bin];
            const float im = mIm[(size_t)bin];
            // 幅度归一化到 0~1（正弦满幅 ≈ 1.0）
            const float magnitude = 2.0f * std::sqrt(re * re + im * im) / (float)kFftSize;
            maxMagnitude = std::max(maxMagnitude, magnitude);
        }

        const float db = 20.0f * std::log10(maxMagnitude + 1.0e-9f);
        float target = (db - kDbFloor) / (kDbCeil - kDbFloor);
        target = std::clamp(target, 0.0f, 1.0f);

        const float current = mBands[band];
        const float coeff = target > current ? kAttack : kRelease;
        const float smoothed = current + (target - current) * coeff;
        mBands[band] = smoothed;
        mPeaks[band] = std::max(smoothed, mPeaks[band] - kPeakDecay);
    }

    SpectrumShm::writeBegin(frame);
    std::memcpy(frame.bands, mBands.data(), sizeof(float) * SpectrumShm::kBandCount);
    std::memcpy(frame.peaks, mPeaks.data(), sizeof(float) * SpectrumShm::kBandCount);
    frame.header.sampleRate = sampleRate;
    frame.header.updatedMs = (uint32_t)juce::Time::currentTimeMillis();
    frame.header.enabled = 1;
    SpectrumShm::writeEnd(frame);
}

void SpectrumAnalyzer::fftRadix2(std::vector<float>& re, std::vector<float>& im) {
    const int n = (int)re.size();
    if (n <= 1) return;

    // 位反转置换
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(re[(size_t)i], re[(size_t)j]);
            std::swap(im[(size_t)i], im[(size_t)j]);
        }
    }

    for (int len = 2; len <= n; len <<= 1) {
        const float angle = -2.0f * juce::MathConstants<float>::pi / (float)len;
        const float stepRe = std::cos(angle);
        const float stepIm = std::sin(angle);

        for (int i = 0; i < n; i += len) {
            float twiddleRe = 1.0f;
            float twiddleIm = 0.0f;

            for (int k = 0; k < len / 2; ++k) {
                const int evenIndex = i + k;
                const int oddIndex = i + k + len / 2;

                const float oddRe = re[(size_t)oddIndex] * twiddleRe -
                                    im[(size_t)oddIndex] * twiddleIm;
                const float oddIm = re[(size_t)oddIndex] * twiddleIm +
                                    im[(size_t)oddIndex] * twiddleRe;

                const float evenRe = re[(size_t)evenIndex];
                const float evenIm = im[(size_t)evenIndex];

                re[(size_t)evenIndex] = evenRe + oddRe;
                im[(size_t)evenIndex] = evenIm + oddIm;
                re[(size_t)oddIndex] = evenRe - oddRe;
                im[(size_t)oddIndex] = evenIm - oddIm;

                const float nextRe = twiddleRe * stepRe - twiddleIm * stepIm;
                twiddleIm = twiddleRe * stepIm + twiddleIm * stepRe;
                twiddleRe = nextRe;
            }
        }
    }
}
