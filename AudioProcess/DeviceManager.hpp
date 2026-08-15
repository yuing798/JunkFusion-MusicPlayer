#pragma once

#include "constants.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_audio_utils/juce_audio_utils.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include <memory>
#include <zmq.hpp>
class DeviceManager : public juce::ChangeListener { // 监听声卡变动
private:
    juce::AudioDeviceManager mManager;                       // 管理声卡
    juce::AudioProcessorPlayer mPlayer;                      // 传递音频
    juce::AudioDeviceManager::AudioDeviceSetup currentSetup; // 当前的声卡设置
    zmq::socket_t& mSocket;
    std::unique_ptr<juce::XmlElement> setupXml;
    juce::File mConfigFile;

public:
    explicit DeviceManager(zmq::socket_t& socket, juce::File configFile);
    // 监听声卡状态变换
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // 连接processor
    void connectProcessor(juce::AudioProcessor* p);

    // 断开processor的连接
    void disconnectProcessor();

    void saveSetupXml2File();

    // AudioDeviceManager::getCpuUsage()
    // 测量的是：“你的音频回调函数（processBlock）每次执行所花费的时间，占声卡给你的极限时间的百分比。”
    juce::var getAvailDeviceType();
    juce::var getAvailDevice(juce::AudioIODeviceType* type);

    DONT_COPY_AND_MOVE(DeviceManager)
};