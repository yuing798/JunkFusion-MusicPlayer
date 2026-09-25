#include "./DeviceManager.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/constants.h"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>


DeviceManager::DeviceManager(juce::File configFile) : mConfigFile(configFile) {

    mManager.addChangeListener(this);

    setupXml = juce::parseXML(configFile);
    OtherUtils::writeEmergencyLog("准备初始化声卡配置");

    juce::String error = mManager.initialise(0, 256, setupXml.get(), true);
    // if (error.isNotEmpty()) {
    //     spdlog::get(LogAudioID)->error("声卡初始化错误!原因:{}", error.toStdString());
    //     // mManager.initialise(0,256,nullptr)
    // }
    currentSetup = mManager.getAudioDeviceSetup();
    // mManager.setAudioDeviceSetup(mManager.getAudioDeviceSetup(), true);
    // setupXml = mManager.createStateXml();
    // saveSetupXml2File();
    // spdlog::get(LogAudioID)->debug("当前的缓冲区长度为:{}", currentSetup.bufferSize);
    // auto bufferSizeList = mManager.getCurrentAudioDevice()->getAvailableBufferSizes();
    // std::string bsStr;
    // for (auto& bs : bufferSizeList) {
    //     bsStr += std::to_string(bs) + "       ";
    // }
    // spdlog::get(LogAudioID)->debug("可设置的缓冲区长度共有:{}", bsStr);

    // {
    //     currentSetup.bufferSize = 1920;
    //     // 这个函数在应用第一次开启的时候是没有用的，只有真正改变了数值大小才有用
    //     mManager.setAudioDeviceSetup(currentSetup, true);
    //     setupXml = mManager.createStateXml();
    //     saveSetupXml2File();
    // }

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
    obj.getDynamicObject()->setProperty(AudioMacro::deviceTypeList, juce::var(deviceTypeList));
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
    obj.getDynamicObject()->setProperty(AudioMacro::deviceList, juce::var(nameArray));
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
        obj.getDynamicObject()->setProperty(AudioMacro::sampleRateList, juce::var(array));
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
        obj.getDynamicObject()->setProperty(AudioMacro::bufferSizeList, juce::var(array));
        return obj;
    }
    return {};
}