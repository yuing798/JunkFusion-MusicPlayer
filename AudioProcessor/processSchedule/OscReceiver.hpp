#pragma once

#include "juce_osc/juce_osc.h"
#include <functional>
class OscReceiver : public juce::OSCReceiver::Listener<juce::OSCReceiver::RealtimeCallback> {
    // 未来可以改成juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::RealtimeCallback>
    // 继承类来做分级监听制度
private:
    juce::OSCReceiver mReceiver;
    int mPort;

public:
    OscReceiver(int oscPort);
    ~OscReceiver();
    void oscMessageReceived(const juce::OSCMessage& message) override;
    void oscBundleReceived(const juce::OSCBundle& bundle) override; // 把多个参数打包在一起发过来

    std::function<void(float)> onMasterVolumeChange;
    std::function<void(float)> onSpeedShifterChange;
    std::function<void(float)> onPitchShifterChange;
};