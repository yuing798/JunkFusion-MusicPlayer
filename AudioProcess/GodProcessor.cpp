#include "./GodProcessor.hpp"
#include "./AudioDefs.hpp"
#include "AudioUtils.hpp"
#include "DeviceManager.hpp"
#include "constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

GodProcessor::GodProcessor(juce::StringArray initArgs)
    : decoderRingBuffer(1000), decoder(decoderRingBuffer) {

    std::string pushPullPort;
    std::string pubSubPort;
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
            auto oscPort = initArgs[i + 1].getIntValue();
            mOscReceiver = std::make_unique<OscReceiver>(oscPort);
            continue;
        }
        if (initArgs[i] == AudioDefs::pubSubPort) {
            pubSubPort = initArgs[i + 1].toStdString();
            continue;
        }
    }
    // 初始化工作者
    mAudioProcessWorker =
        std::make_unique<AudioProcessWorker>(std::move(pushPullPort), std::move(pubSubPort));

    // 初始化设备管理者
    juce::File configFile = mCacheDir.getChildFile("JFConfig.xml");
    if (!configFile.existsAsFile()) configFile.create();
    mDeviceManager = std::make_unique<DeviceManager>(std::move(configFile));

    // 初始化日志
    AudioUtils::initAudioLogger(mCacheDir);

    auto log{spdlog::get(LogAudioID)};
    log->debug("音频进程日志初始化完成");

    // 解码失败弹窗
    decoder.sendErrorMsg = [this](std::string str) {
        str += "FFmpeg解码错误:";
        juce::var obj{new juce::DynamicObject()};
        obj.getDynamicObject()->setProperty(AudioDefs::errorPopupWindowMsg, juce::String(str));
        auto jsonStr{juce::JSON::toString(obj).toStdString()};

        mAudioProcessWorker->sender->sendMessage(jsonStr);
    };
}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    decoder.prepareToPlay(outputLayout, sampleRate);
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    buffer.clear();
    juce::ignoreUnused(midiMessages);
    decoderRingBuffer.popAudioData(buffer);
}
void GodProcessor::setStateInformation(const void* data, int sizeInBytes) {}
void GodProcessor::getStateInformation(juce::MemoryBlock& destData) {}