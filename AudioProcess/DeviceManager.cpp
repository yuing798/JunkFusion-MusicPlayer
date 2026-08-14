#include "./DeviceManager.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>
#include <zmq.hpp>

DeviceManager::DeviceManager(zmq::socket_t& socket, juce::File configFile)
    : mSocket(socket), mConfigFile(configFile) {

    if (configFile.existsAsFile()) {
        xml = juce::parseXML(configFile);
    }

    // 第三个参数传入 xml.get()。
    // 如果 xml 不为空，JUCE 会优先按照 XML 里的配置打开声卡；如果 xml 为空，则自动使用默认设备。
    juce::String error = mManager.initialise(0, 2, xml.get(), true);

    if (error.isNotEmpty()) {
        juce::Logger::writeToLog("初始化声卡失败: " + error);
    }

    mManager.initialise(0, 256, nullptr, true);
}

void DeviceManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &mManager) {
        // 这个if在底层声卡参数“改变完成并生效后”才会调用的，而不是在调节过程中
        currentSetup = mManager.getAudioDeviceSetup();
    }
}

void DeviceManager::connectProcessor(juce::AudioProcessor* p) {
    mPlayer.setProcessor(p);
    mManager.addAudioCallback(&mPlayer);
}
void DeviceManager::disconnectProcessor() {
    mManager.removeAudioCallback(&mPlayer);
    mPlayer.setProcessor(nullptr);
}

void DeviceManager::saveSetupXml2File(juce::File cacheDir) {
    setupXml = mManager.createStateXml();

    if (setupXml != nullptr) {
        configFile = cacheDir.getChildFile("deviceConfig.xml");

        if (!configFile.existsAsFile()) configFile.create();

        // 4. 将 XmlElement 直接写入本地文件
        bool success = setupXml->writeTo(configFile);

        auto log = spdlog::get(LogAudioID);
        if (success) {
            log->debug("声卡配置已保存到: " + configFile.getFullPathName());
        } else {
            log->error("错误：无法写入声卡配置文件！");
        }
    }
}