#include "./SpectrumController.hpp"

#include "Macro/audioMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <cstring>
#include <spdlog/spdlog.h>

namespace {
/// 共享内存的基名；后面拼进程号，避免同时开两个实例时撞车
constexpr const char* kShmBaseName = "JunkFusion.Spectrum.";
} // namespace

uint32_t SpectrumController::currentProcessId() noexcept {
#ifdef _WIN32
    return static_cast<uint32_t>(::GetCurrentProcessId());
#else
    return static_cast<uint32_t>(::getpid());
#endif
}

void SpectrumController::init(const std::function<void(const std::string&)>& send) {
    mSend = send;

    const auto pid = currentProcessId();
    const std::string baseName = std::string{kShmBaseName} + std::to_string(pid);

    mShm = std::make_unique<SharedMemory>();
    if (!mShm->create(baseName, SpectrumShm::kByteSize)) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("SpectrumController::init 创建共享内存失败:{}", baseName);
        mShm.reset();
        return;
    }

    mFrame = static_cast<SpectrumShm::Frame*>(mShm->data());
    SpectrumShm::initialise(*mFrame);

    if (!mSend) return;

    Yvar inner = Yvar::object();
    inner.set(AudioMacro::spectrumName, mShm->fullName());
    inner.set(AudioMacro::spectrumBytes, static_cast<int64_t>(SpectrumShm::kByteSize));

    Yvar envelope = Yvar::object();
    envelope.set(AudioMacro::spectrumShm, inner);

    mSend(envelope.dump());

    if (auto logger = spdlog::get(LogDllID))
        logger->debug("频谱共享内存已创建并通知音频进程:{}", mShm->fullName());
}

void SpectrumController::setEnabled(bool enabled) {
    if (!mSend) return;

    Yvar obj = Yvar::object();
    obj.set(AudioMacro::spectrumEnable, enabled);
    mSend(obj.dump());
}

bool SpectrumController::refresh() {
    if (mFrame == nullptr) return false;
    if (!SpectrumShm::isValidFrame(*mFrame)) return false;

    // seqlock：最多重试几次，读不到就放弃这一帧（下一帧再来）
    for (int attempt = 0; attempt < 3; ++attempt) {
        uint32_t sequence = 0;
        if (!SpectrumShm::readBegin(*mFrame, sequence)) continue;

        std::memcpy(mBands.data(), mFrame->bands, sizeof(float) * SpectrumShm::kBandCount);
        std::memcpy(mPeaks.data(), mFrame->peaks, sizeof(float) * SpectrumShm::kBandCount);
        const uint32_t enabled = mFrame->header.enabled;

        if (!SpectrumShm::readEnd(*mFrame, sequence)) continue;

        if (enabled == 0) return false; // 音频进程说现在没在分析
        return true;
    }
    return false;
}

const float* SpectrumController::getBands() {
    return refresh() ? mBands.data() : nullptr;
}

const float* SpectrumController::getPeaks() {
    return refresh() ? mPeaks.data() : nullptr;
}
