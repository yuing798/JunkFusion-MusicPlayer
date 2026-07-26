#pragma once

#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_events/juce_events.h"
class DeviceManager : public juce::ChangeListener {
private:
    juce::AudioDeviceManager manager;

public:
    // 监听声卡状态变换
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
};