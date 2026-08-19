#include "./DeviceManager.hpp"
#include "AudioDefs.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "otherUtils.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <zmq.hpp>

DeviceManager::DeviceManager(juce::File configFile) : mConfigFile(configFile) {

    mManager.addChangeListener(this);

    setupXml = juce::parseXML(configFile);
    Utils::writeEmergencyLog("准备初始化声卡配置");

    // mManager.initialiseWithDefaultDevices(0, 2);
    // auto Currentsetup = mManager.getAudioDeviceSetup();
    // auto currentAudioType = mManager.getCurrentAudioDeviceType();
    // spdlog::get(LogAudioID)->debug("现在的音频驱动类型为:{}", currentAudioType.toStdString());
    // auto currentDevice = Currentsetup.outputDeviceName;
    // spdlog::get(LogAudioID)->debug("现在的设备为:{}", currentDevice.toStdString());
    // spdlog::get(LogAudioID)->debug("现在的设备采样率为:{}", Currentsetup.sampleRate);

    juce::String error = mManager.initialise(0, 256, setupXml.get(), true);
    currentSetup = mManager.getAudioDeviceSetup();
    mManager.setAudioDeviceSetup(mManager.getAudioDeviceSetup(), true);
    setupXml = mManager.createStateXml();
    saveSetupXml2File();
    spdlog::get(LogAudioID)->debug("当前的缓冲区长度为:{}", currentSetup.bufferSize);
    auto bufferSizeList = mManager.getCurrentAudioDevice()->getAvailableBufferSizes();
    std::string bsStr;
    for (auto& bs : bufferSizeList) {
        bsStr += std::to_string(bs) + "       ";
    }
    spdlog::get(LogAudioID)->debug("可设置的缓冲区长度共有:{}", bsStr);

    currentSetup.bufferSize = 1920;
    mManager.setAudioDeviceSetup(currentSetup, true);
    setupXml = mManager.createStateXml();
    saveSetupXml2File();

    currentSetup.bufferSize = 441;
    mManager.setAudioDeviceSetup(currentSetup, true);
    setupXml = mManager.createStateXml();
    saveSetupXml2File();

    currentSetup.bufferSize = 1920;
    mManager.setAudioDeviceSetup(currentSetup, true);
    setupXml = mManager.createStateXml();
    saveSetupXml2File();

    // auto futuresetup = mManager.getAudioDeviceSetup();

    // auto futureAudioType = mManager.getCurrentAudioDeviceType();
    // spdlog::get(LogAudioID)->debug("未来的音频驱动类型为:{}", futureAudioType.toStdString());
    // auto futureDevice = futuresetup.outputDeviceName;
    // spdlog::get(LogAudioID)->debug("未来的设备为:{}", futureDevice.toStdString());
    // spdlog::get(LogAudioID)->debug("未来的设备采样率为:{}", futuresetup.sampleRate);

    if (error.isNotEmpty()) {
        spdlog::get(LogAudioID)->error("声卡初始化失败: {}", error.toStdString());
    }

    // Utils::writeEmergencyLog("DeviceManager构造函数完成");
}

void DeviceManager::saveSetupXml2File() {
    auto log = spdlog::get(LogAudioID);

    // log->debug("准备保存声卡配置");

    // auto setup = mManager.getAudioDeviceSetup();

    // log->debug("当前设备: {}", setup.outputDeviceName.toStdString());

    // log->debug("sampleRate: {}", setup.sampleRate);

    // log->debug("bufferSize: {}", setup.bufferSize);

    // auto xml = mManager.createStateXml();

    if (setupXml == nullptr) {
        log->error("缓存失败!AudioDeviceManager::createStateXml() 返回 nullptr");
        return;
    }

    log->debug("生成 XML:\n{}", setupXml->toString().toStdString());

    if (!setupXml->writeTo(mConfigFile)) {
        log->error("无法写入声卡配置文件: {}", mConfigFile.getFullPathName().toStdString());
        return;
    }

    log->debug("声卡配置已保存到: {}", mConfigFile.getFullPathName().toStdString());
}

DeviceManager::~DeviceManager() { mManager.removeChangeListener(this); }

void DeviceManager::changeListenerCallback(juce::ChangeBroadcaster* source) {
    if (source == &mManager) {
        // 这个if在底层声卡参数“改变完成并生效后”才会调用的，而不是在调节过程中
        // currentSetup = mManager.getAudioDeviceSetup();
        // saveSetupXml2File();

        // auto* currentDevice = mManager.getCurrentAudioDevice();
        // auto log = spdlog::get(LogAudioID);

        // if (currentDevice != nullptr) {
        //     // 设备真正打开了！
        //     log->debug("底层声卡已生效，当前挂载设备: {}",
        //     currentDevice->getName().toStdString()); currentSetup =
        //     mManager.getAudioDeviceSetup(); saveSetupXml2File();
        // } else {
        //     // 根本没有声卡被打开
        //     log->error("回调触发，但底层没有打开任何物理声卡！");
        // }
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