#include "./EqController.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <utility>

void EqController::init(ParamLink::OscSend osc, ParamLink::JsonSend json) {
    mLink.setTransports(std::move(osc), std::move(json));
}

void EqController::updateBandLive(int idx, float freqHz, float gainDb, float q, float shelfSlope) {
    if (idx < 0 || idx >= EqBandParam::kMaxBands) return;

    // 本地状态也要跟着走，否则前端读回时会看到旧值
    auto& band = mState.bands[static_cast<size_t>(idx)];
    band.freqHz = freqHz;
    band.gainDb = gainDb;
    band.q = q;
    band.shelfSlope = shelfSlope;

    // OSC 地址带上 band 下标：/eq/band/<idx>
    char address[32] = {};
    std::snprintf(address, sizeof(address), "/eq/band/%d", idx);
    mLink.sendLive(address, {freqHz, gainDb, q, shelfSlope});
}

void EqController::setBand(const EqBandParam& band) {
    if (band.idx < 0 || band.idx >= EqBandParam::kMaxBands) return;

    EqBandParam param = band;
    mState.bands[static_cast<size_t>(param.idx)] = param;
    mLink.sendCommit("eqBand", EqWire::encodeBand(param));
}

void EqController::setMaster(bool enabled, float masterGainDb) {
    mState.enabled = enabled;
    mState.masterGainDb = masterGainDb;
    mLink.sendCommit("eqMaster", EqWire::encodeMaster(enabled, masterGainDb));
}

void EqController::setAll(const EqAllParam& all) {
    mState = all;
    mLink.sendCommit("eqAll", EqWire::encodeAll(all));
}

void EqController::setSampleRate(double sampleRate) {
    if (sampleRate > 0.0) mSampleRate = sampleRate;
}

const float* EqController::getCurve(int numPoints) {
    const int points = std::clamp(numPoints, kCurveMinPoints, kCurveMaxPoints);

    // 先把 8 条 band 译成系数（每条最多 2 段），之后只做幅度求值。
    // 不能每个频点都重算一次系数：那样每帧要跑 2048 次 sin/cos/pow。
    std::array<
        std::array<EqFilterMath::Coefficients, EqFilterMath::kMaxSections>,
        EqBandParam::kMaxBands>
        sections{};
    for (int b = 0; b < EqBandParam::kMaxBands; ++b) {
        EqFilterMath::buildBand(
            mState.bands[static_cast<size_t>(b)],
            mSampleRate,
            sections[static_cast<size_t>(b)]
        );
    }

    mCurve.resize(static_cast<size_t>(points));
    const double ratio = kCurveMaxHz / kCurveMinHz;

    for (int i = 0; i < points; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(points - 1);
        const double freq = kCurveMinHz * std::pow(ratio, t);

        double totalDb = 0.0;
        for (int b = 0; b < EqBandParam::kMaxBands; ++b) {
            if (!mState.bands[static_cast<size_t>(b)].enabled) continue;
            totalDb += EqFilterMath::sectionsMagnitudeDb(
                sections[static_cast<size_t>(b)],
                freq,
                mSampleRate
            );
        }
        mCurve[static_cast<size_t>(i)] = static_cast<float>(totalDb);
    }

    return mCurve.data();
}
