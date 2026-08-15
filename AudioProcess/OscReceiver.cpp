#include "./OscReceiver.hpp"
#include "constants.h"
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
    juce::String msg = message.getAddressPattern().toString();
}

void OscReceiver::oscBundleReceived(const juce::OSCBundle& bundle) {}
