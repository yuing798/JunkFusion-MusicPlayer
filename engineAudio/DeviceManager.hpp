#pragma once

#include "GodProcessor.hpp"
#include "constants.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_events/juce_events.h"
class DeviceManager : public juce::ChangeListener {
private:
    juce::AudioDeviceManager manager;
    juce::AudioProcessorPlayer player;

public:
    // 监听声卡状态变换
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // 连接processor
    void connectProcessor(GodProcessor* p);

    // 断开processor的连接
    void disconnectProcessor();

    DONT_COPY_AND_MOVE(DeviceManager)
};