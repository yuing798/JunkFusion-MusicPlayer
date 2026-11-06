#include "./paramLink.hpp"
#include "Utils/Yvar.hpp"
#include <chrono>
#include <utility>

void ParamLink::setTransports(OscSend osc, JsonSend json) {
    mOsc = std::move(osc);
    mJson = std::move(json);
}

void ParamLink::setRateLimitHz(double hz) {
    mMinIntervalSec = (hz > 0.0) ? (1.0 / hz) : 0.0;
}

double ParamLink::getRateLimitHz() const noexcept {
    return mMinIntervalSec > 0.0 ? 1.0 / mMinIntervalSec : 0.0;
}

ParamLink::Slot* ParamLink::findSlot(const char* address) {
    Slot* freeSlot = nullptr;
    for (auto& slot : mSlots) {
        if (slot.address == address) return &slot;
        if (freeSlot == nullptr && slot.address.empty()) freeSlot = &slot;
    }
    if (freeSlot != nullptr) freeSlot->address = address;
    return freeSlot; // 槽位满了返回 nullptr：下面按"不节流直发"处理
}

void ParamLink::sendLive(const char* address, std::initializer_list<float> values) {
    if (address == nullptr || !mOsc) return;

    const double nowSec =
        std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();

    if (Slot* slot = findSlot(address)) {
        if (slot->lastSentSec >= 0.0 && (nowSec - slot->lastSentSec) < mMinIntervalSec) {
            return; // 节流：这一帧直接丢掉，最新一帧马上会来
        }
        slot->lastSentSec = nowSec;
    }

    mOsc(address, values.begin(), static_cast<int>(values.size()), ++mRev);
}

void ParamLink::sendCommit(const char* key, const Yvar& payload) {
    if (key == nullptr || !mJson) return;

    Yvar envelope = Yvar::object();
    envelope.set(key, payload);
    envelope.set("rev", static_cast<int64_t>(++mRev));
    mJson(envelope);
}
