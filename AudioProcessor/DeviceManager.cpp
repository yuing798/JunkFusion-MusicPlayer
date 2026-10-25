#include "./DeviceManager.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>

DeviceManager::DeviceManager(juce::File configFile) : mConfigFile(configFile) {

    if (!configFile.existsAsFile()) configFile.create();

    mManager.addChangeListener(this);

    setupXml = juce::parseXML(configFile);
    OtherUtils::writeEmergencyLog("准备初始化声卡配置");

    juce::String error = mManager.initialise(0, 256, setupXml.get(), true);
    currentSetup = mManager.getAudioDeviceSetup();

    if (error.isNotEmpty()) {
        spdlog::get(LogAudioID)->error("声卡初始化失败: {}", error.toStdString());
    }

    // OtherUtils::writeEmergencyLog("DeviceManager构造函数完成");
    spdlog::get(LogAudioID)->debug("DeviceManager构造函数完成");
}

void DeviceManager::saveSetupXml2File() {
    auto log = spdlog::get(LogAudioID);

    if (setupXml == nullptr) {
        log->error(
            "缓存失败!AudioDeviceManager::createStateXml() 返回 "
            "nullptr,可能是应用第一次初始化的原因"
        );
        return;
    }

    log->debug("生成 XML:\n{}", setupXml->toString().toStdString());

    if (!setupXml->writeTo(mConfigFile)) {
        log->error("无法写入声卡配置文件: {}", mConfigFile.getFullPathName().toStdString());
        return;
    }

    log->debug("声卡配置已保存到: {}", mConfigFile.getFullPathName().toStdString());
}

DeviceManager::~DeviceManager() {
    mManager.removeChangeListener(this);
    disconnectProcessor();
}

void DeviceManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &mManager) {
        spdlog::get(LogAudioID)->debug("触发了DeviceManager::changeListenerCallback");
        setupXml = mManager.createStateXml();
        saveSetupXml2File();
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

Yvar DeviceManager::getAvailDeviceType() {
    const auto& types = mManager.getAvailableDeviceTypes();

    std::string logInfo = "检测到的驱动类型有:";
    Yvar deviceTypeList = Yvar::array();

    for (auto* type : types) {
        auto typeName = type->getTypeName();
        logInfo += typeName.toStdString();
        deviceTypeList.push_back(typeName.toStdString());
    }

    Yvar obj = Yvar::object();
    obj.set(AudioMacro::deviceTypeList, std::move(deviceTypeList));
    return obj;
}
Yvar DeviceManager::getAvailDevice(juce::AudioIODeviceType* type) {
    if (type == nullptr) return Yvar::object();
    auto deviceNames = type->getDeviceNames();
    // 将 deviceNames 转换为 Yvar
    Yvar nameArray = Yvar::array();
    for (const auto& name : deviceNames)
        nameArray.push_back(name.toStdString());
    Yvar obj = Yvar::object();
    obj.set(AudioMacro::deviceList, nameArray);
    return obj;
}
Yvar DeviceManager::getAvailSampleRateList() {
    if (auto* device = mManager.getCurrentAudioDevice()) {
        auto sampleRateList = device->getAvailableSampleRates();
        Yvar array = Yvar::array();
        for (auto& sr : sampleRateList) {
            array.push_back(sr); // 保留两位小数
        }
        Yvar obj = Yvar::object();
        obj.set(AudioMacro::sampleRateList, array);
        return obj;
    }
    return {};
}

Yvar DeviceManager::getAvailBufferSizeList() {
    if (auto* device = mManager.getCurrentAudioDevice()) {
        auto bufferSizeList = device->getAvailableBufferSizes();
        auto array = Yvar::array();
        for (auto& bufferSize : bufferSizeList) {
            array.push_back(bufferSize);
        }
        auto obj = Yvar::object();
        obj.set(AudioMacro::bufferSizeList, array);
        return obj;
    }
    return {};
}