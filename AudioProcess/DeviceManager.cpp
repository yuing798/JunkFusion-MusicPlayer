#include "./DeviceManager.hpp"
#include "AudioDefs.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>

DeviceManager::DeviceManager(juce::File configFile) : mConfigFile(configFile) {

    setupXml = juce::parseXML(configFile);

    // 如果 xml 不为空，JUCE 会优先按照 XML 里的配置打开声卡；如果 xml 为空，则自动使用默认设备。
    juce::String error = mManager.initialise(0, 256, setupXml.get(), true);

    if (error.isNotEmpty()) {
        auto log{spdlog::get(LogAudioID)};
        log->error("初始化声卡失败: {}", error.toStdString());
    }
    mManager.addChangeListener(this);
}

DeviceManager::~DeviceManager() { mManager.removeChangeListener(this); }

void DeviceManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &mManager) {
        // 这个if在底层声卡参数“改变完成并生效后”才会调用的，而不是在调节过程中
        currentSetup = mManager.getAudioDeviceSetup();
    }
}

void DeviceManager::connectProcessor(juce::AudioProcessor* p) {
    jassert(p);
    mPlayer.setProcessor(p);
    mManager.addAudioCallback(&mPlayer);
}
void DeviceManager::disconnectProcessor() {
    mManager.removeAudioCallback(&mPlayer);
    mPlayer.setProcessor(nullptr);
}

void DeviceManager::saveSetupXml2File() {
    setupXml = mManager.createStateXml();

    if (setupXml != nullptr) {

        // 4. 将 XmlElement 直接写入本地文件
        bool success = setupXml->writeTo(mConfigFile);

        auto log = spdlog::get(LogAudioID);
        if (success) {
            log->debug("声卡配置已保存到: {}", mConfigFile.getFullPathName().toStdString());
        } else {
            log->error("错误：无法写入声卡配置文件！");
        }
    }
}

juce::var DeviceManager::getAvailDeviceType() {
    // 获取所有的驱动类型集合 (比如 ASIO, WASAPI, DirectSound)
    const auto& types{mManager.getAvailableDeviceTypes()};
    std::string logInfo{"检测到的驱动类型有:"};
    juce::Array<juce::var> deviceTypeList;

    for (auto* type : types) {
        auto typeName = type->getTypeName();
        logInfo += typeName.toStdString();
        deviceTypeList.add(typeName);
    }
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AudioDefs::deviceTypeList, juce::var(deviceTypeList));
    return obj;
}
juce::var DeviceManager::getAvailDevice(juce::AudioIODeviceType* type) {
    if (type == nullptr) return juce::var();
    auto deviceNames = type->getDeviceNames();
    // 将 deviceNames 转换为 juce::var
    juce::Array<juce::var> nameArray;
    for (const auto& name : deviceNames)
        nameArray.add(name);
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AudioDefs::deviceList, juce::var(nameArray));
    return obj;
}
juce::var DeviceManager::getAvailSampleRateList() {
    if (auto* device = mManager.getCurrentAudioDevice()) {
        auto sampleRateList = device->getAvailableSampleRates();
        juce::Array<juce::var> array;
        for (auto& sr : sampleRateList) {
            array.add(juce::String(sr, 2)); // 保留两位小数
        }
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(AudioDefs::sampleRateList, juce::var(array));
        return obj;
    }
    return {};
}

juce::var DeviceManager::getAvailBufferSizeList() {
    if (auto* device = mManager.getCurrentAudioDevice()) {
        auto bufferSizeList = device->getAvailableBufferSizes();
        juce::Array<juce::var> array;
        for (auto& bufferSize : bufferSizeList) {
            array.add(bufferSize);
        }
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(AudioDefs::bufferSizeList, juce::var(array));
        return obj;
    }
    return {};
}