#include "./GodProcessor.hpp"
#include "AudioPreProcess.hpp"
#include "AudioRingBuffer.hpp"
#include "DeviceManager.hpp"
#include "Macro/audioMacro.hpp"
#include "PlayCount.hpp"
#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

GodProcessor::GodProcessor(juce::StringArray initArgs) {

    std::string pushPullPort;
    std::string pubSubPort;
    int oscPort{0};
    for (int i = 0; i < initArgs.size(); i++) {
        if (initArgs[i] == AudioMacro::pushPullPort) {
            pushPullPort = initArgs[i + 1].toStdString();
            continue;
        }
        if (initArgs[i] == AudioMacro::cacheDir) {
            mCacheDir = juce::File{initArgs[i + 1]};
            continue;
        }
        if (initArgs[i] == AudioMacro::oscPort) {
            // 初始化osc接收者
            oscPort = initArgs[i + 1].getIntValue();

            continue;
        }
        if (initArgs[i] == AudioMacro::pubSubPort) {
            pubSubPort = initArgs[i + 1].toStdString();
            continue;
        }
    }
    // Utils::writeEmergencyLog("准备执行worker的构造函数");
    // 初始化工作者
    mAudioProcessWorker =
        std::make_unique<AudioProcessWorker>(std::move(pushPullPort), std::move(pubSubPort));

    mPreProcess = std::make_unique<AudioPreProcess>(mAudioProcessWorker.get());

    mOscReceiver = std::make_unique<OscReceiver>(oscPort);
    mStretch = std::make_unique<Stretch>(mAudioProcessWorker.get(), mOscReceiver.get());

    mAudioProcessWorker->receiver->onMasterVolumeChange = [this](float value) {
        masterVolume.setTargetValue(value);
    };

    mOscReceiver->onMasterVolumeChange = [this](float value) {
        masterVolume.setTargetValue(value);
    };

    // OtherUtils::writeEmergencyLog("GodProcessor构造函数执行完成");

    // 初始化设备管理者
    {
        juce::File configFile = mCacheDir.getChildFile("JFConfig.xml");
        if (!configFile.existsAsFile()) configFile.create();
        mDeviceManager = std::make_unique<DeviceManager>(std::move(configFile));
        mDeviceManager->connectProcessor(this); // 设备链接必须放到构造函数的末尾，绝对不能移动
    }
}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    mSampleRate = sampleRate;

    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    mNumChannels = outputLayout.size();
    mPreProcess->prepareToPlay(outputLayout, sampleRate, maximumExpectedSamplesPerBlock);

    masterVolume.reset(sampleRate, 0.002);
    masterVolume.setCurrentAndTargetValue(1.0);

    mSpeedShifterBuffer.setSize(mNumChannels, maximumExpectedSamplesPerBlock * 2);
    mStretch->prepareToPlay(sampleRate, mNumChannels, maximumExpectedSamplesPerBlock);
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    auto finalNumSamples{buffer.getNumSamples()}; // 最终需要的缓冲区大小
    auto speedShifterNumSamples{
        static_cast<int>(finalNumSamples * mStretch->getSpeedShifterValue())
    }; // 解码-->变速之间都要根据这个缓冲区大小做计算

    mPreProcess->processBlock(mSpeedShifterBuffer, speedShifterNumSamples);
    // buffer.clear();
    mStretch->processBlock(mSpeedShifterBuffer, speedShifterNumSamples, buffer, finalNumSamples);
    // for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
    //     buffer.copyFrom(ch, 0, mSpeedShifterBuffer.getReadPointer(ch), finalNumSamples);
    // } // 这个做个测试

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        auto currentMasterVolume{masterVolume.getNextValue()};
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto ptr{buffer.getWritePointer(ch)};
            ptr[i] *= currentMasterVolume;
        }
    }

    // static int mA = 0, mNeed = 0, mCnt = 0;
    // mA = 0;
    // mNeed = 0;
    // for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    //     mA = juce::jmax(mA, (int)buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    // mNeed = (int)(buffer.getNumSamples() * mStretch->getSpeedShifterValue());
    // if (++mCnt == 200) {
    //     std::string logStr{
    //         "B=" + std::to_string(buffer.getNumSamples()) + " Nch=" +
    //         std::to_string(mNumChannels) + " inCh=" +
    //         std::to_string(mSpeedShifterBuffer.getNumChannels()) + " shifterLen=" +
    //         std::to_string(mSpeedShifterBuffer.getNumSamples()) + " need=" +
    //         std::to_string(mNeed) + " outMag=" + std::to_string(mA)
    //     };
    //     spdlog::get(LogAudioID)->debug(logStr);
    // }
}

GodProcessor::~GodProcessor() {}

void GodProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::ignoreUnused(data);
    juce::ignoreUnused(sizeInBytes);
}
void GodProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::ignoreUnused(destData);
}

bool GodProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    auto outSet = layouts.getMainOutputChannelSet();
    // 只要输出不是禁用状态，且通道数在 1~256 之间，就允许声卡进行绑定
    return !outSet.isDisabled() && outSet.size() > 0 && outSet.size() <= 256;
}