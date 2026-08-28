#include "./GodProcessor.hpp"
#include "./AudioDefs.hpp"
#include "AudioPreProcess.hpp"
#include "AudioRingBuffer.hpp"
#include "AudioUtils.hpp"
#include "DeviceManager.hpp"
#include "constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

GodProcessor::GodProcessor(juce::StringArray initArgs) {

    // Utils::writeEmergencyLog("开始执行GodProcessor的构造函数");

    std::string pushPullPort;
    std::string pubSubPort;
    int oscPort{0};
    for (int i = 0; i < initArgs.size(); i++) {
        if (initArgs[i] == AudioDefs::pushPullPort) {
            pushPullPort = initArgs[i + 1].toStdString();
            continue;
        }
        if (initArgs[i] == AudioDefs::cacheDir) {
            mCacheDir = juce::File{initArgs[i + 1]};
            continue;
        }
        if (initArgs[i] == AudioDefs::oscPort) {
            // 初始化osc接收者
            oscPort = initArgs[i + 1].getIntValue();

            continue;
        }
        if (initArgs[i] == AudioDefs::pubSubPort) {
            pubSubPort = initArgs[i + 1].toStdString();
            continue;
        }
    }
    // Utils::writeEmergencyLog("准备执行worker的构造函数");
    // 初始化工作者
    mAudioProcessWorker =
        std::make_unique<AudioProcessWorker>(std::move(pushPullPort), std::move(pubSubPort));

    mPreProcess = std::make_unique<AudioPreProcess>(mAudioProcessWorker.get());

    // Utils::writeEmergencyLog("worker的构造函数执行完毕");

    // 初始化设备管理者
    juce::File configFile = mCacheDir.getChildFile("JFConfig.xml");
    if (!configFile.existsAsFile()) configFile.create();
    mDeviceManager = std::make_unique<DeviceManager>(std::move(configFile));

    mOscReceiver = std::make_unique<OscReceiver>(oscPort);

    mDeviceManager->connectProcessor(this);

    mAudioProcessWorker->receiver->onMasterVolumeChange = [this](float value) {
        masterVolume.setTargetValue(value);
    };

    mOscReceiver->onMasterVolumeChange = [this](float value) {
        masterVolume.setTargetValue(value);
    };
}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    mSampleRate = sampleRate;

    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    mNumChannels = outputLayout.size();
    mPreProcess->prepareToPlay(outputLayout, sampleRate, maximumExpectedSamplesPerBlock);

    masterVolume.reset(sampleRate, 0.002);
    masterVolume.setCurrentAndTargetValue(1.0);
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    mPreProcess->processBlock(buffer);

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        auto currentMasterVolume{masterVolume.getNextValue()};
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto ptr{buffer.getWritePointer(ch)};
            ptr[i] *= currentMasterVolume;
        }
    }
}

GodProcessor::~GodProcessor() {}

void GodProcessor::setStateInformation(const void* data, int sizeInBytes) {}
void GodProcessor::getStateInformation(juce::MemoryBlock& destData) {}

bool GodProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    auto outSet = layouts.getMainOutputChannelSet();
    // 只要输出不是禁用状态，且通道数在 1~256 之间，就允许声卡进行绑定
    return !outSet.isDisabled() && outSet.size() > 0 && outSet.size() <= 256;
}