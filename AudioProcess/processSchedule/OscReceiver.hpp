#pragma once

#include "juce_osc/juce_osc.h"
class OscReceiver : public juce::OSCReceiver::Listener<juce::OSCReceiver::RealtimeCallback> {
private:
    juce::OSCReceiver mReceiver;
    int mPort;

public:
    OscReceiver(int oscPort);
    ~OscReceiver();
    void oscMessageReceived(const juce::OSCMessage& message) override;
    void oscBundleReceived(const juce::OSCBundle& bundle) override; // 把多个参数打包在一起发过来
};