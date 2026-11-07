#include "./OscReceiver.hpp"
#include "Macro/sliderParam.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>

OscReceiver::OscReceiver(int oscPort) : mPort(oscPort) {
    mReceiver.addListener(this);
    auto log{spdlog::get(LogAudioID)};
    if (mReceiver.connect(mPort)) {
        log->debug("osc链接成功");
    } else {
        log->error("osc链接失败");
    }
}
OscReceiver::~OscReceiver() {
    mReceiver.removeListener(this);
    mReceiver.disconnect();
}
void OscReceiver::oscMessageReceived(const juce::OSCMessage& message) {
    juce::String address = message.getAddressPattern().toString();
    if (message.size() <= 0) return;

    // ── EQ band 拖拽（高频连续量，走 UDP 那条通道）──
    // 地址: /eq/band/<idx>，参数: freqHz, gainDb, q, shelfSlope, rev(int32)
    if (address.startsWith("/eq/band/")) {
        if (message.size() < 5) return;
        for (int i = 0; i < 4; ++i) {
            if (!message[i].isFloat32()) return;
        }

        const int idx = address.substring(9).getIntValue(); // 跳过 "/eq/band/"
        const uint32_t rev =
            message[4].isInt32() ? static_cast<uint32_t>(message[4].getInt32()) : 0u;

        if (onEqBandLive) {
            onEqBandLive(
                idx,
                message[0].getFloat32(),
                message[1].getFloat32(),
                message[2].getFloat32(),
                message[3].getFloat32(),
                rev
            );
        }
        return;
    }

    if (address == SliderParam::masterVolume) {
        if (!message[0].isFloat32()) return;
        float masterVolume{message[0].getFloat32()};
        if (onMasterVolumeChange) onMasterVolumeChange(masterVolume);
    }
    if (address == SliderParam::pitchShifter) {
        if (!message[0].isFloat32()) return;
        float value{message[0].getFloat32()};
        if (onPitchShifterChange) onPitchShifterChange(value);
    }
    if (address == SliderParam::speedShifter) {
        if (!message[0].isFloat32()) return;
        float value{message[0].getFloat32()};
        if (onSpeedShifterChange) onSpeedShifterChange(value);
    }
}

void OscReceiver::oscBundleReceived(const juce::OSCBundle& bundle) { juce::ignoreUnused(bundle); }
