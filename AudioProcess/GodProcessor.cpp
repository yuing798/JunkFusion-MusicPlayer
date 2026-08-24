#include "./GodProcessor.hpp"
#include "./AudioDefs.hpp"
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

    Utils::writeEmergencyLog("开始执行GodProcessor的构造函数");

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
    Utils::writeEmergencyLog("准备执行worker的构造函数");
    // 初始化工作者
    mAudioProcessWorker =
        std::make_unique<AudioProcessWorker>(std::move(pushPullPort), std::move(pubSubPort));

    Utils::writeEmergencyLog("worker的构造函数执行完毕");

    // 初始化设备管理者
    juce::File configFile = mCacheDir.getChildFile("JFConfig.xml");
    if (!configFile.existsAsFile()) configFile.create();
    mDeviceManager = std::make_unique<DeviceManager>(std::move(configFile));

    mOscReceiver = std::make_unique<OscReceiver>(oscPort);

    // 解码失败弹窗

    mPreProcess.sendErrorMsg = [this](std::string errorStr) {
        mAudioProcessWorker->sender->sendMessage(errorStr);
    };

    // 播放新歌
    mAudioProcessWorker->receiver->onPlayNewSongInfoReceived =
        [this](std::string songPath, double songDuration) {
            mPreProcess.playNewSong(songPath);
            currentPlaySamples = 0;
        };

    // 继续播放
    mAudioProcessWorker->receiver->onContinuePlay = [this]() { mPreProcess.continuePlay(); };
    // 停止播放
    mAudioProcessWorker->receiver->onPausePlay = [this]() { mPreProcess.pausePlay(); };

    // 移动进度条到目标秒数
    mAudioProcessWorker->receiver->onSeekTargetPTS = [this](double targetSeconds) {
        mPreProcess.seekPreferPTS(targetSeconds);
        currentPlaySamples = targetSeconds * mSampleRate;
    };

    mDeviceManager->connectProcessor(this);

    startTimer(300); // 播放进度条的监听频率
}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    mSampleRate = sampleRate;

    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    mNumChannels = outputLayout.size();
    mPreProcess.prepareToPlay(outputLayout, sampleRate, maximumExpectedSamplesPerBlock);
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);

    mPreProcess.processBlock(buffer);

    // for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
    //     auto* writePointer = buffer.getWritePointer(channel);
    //     for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
    //         if (std::isnan(writePointer[sample]) || std::isinf(writePointer[sample])) {
    //             // 如果进到这里，说明解码器吐出了脏数据！
    //             Utils::writeEmergencyLog("CRITICAL: NaN or Inf detected at sample ");
    //         }
    //     }
    // }

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        if (!mPreProcess.getIsFullMute()) {
            currentPlaySamples++;
        }
    }

    // if (isSongChanging.load()) {
    //     decoderRingBuffer.clip2Smooth(mSmoothMs);
    //     isSongChanging.store(false); // 切换时的淡出效果
    //     // return;
    // }

    // decoderRingBuffer.popAudioData(buffer);
    // Utils::writeEmergencyLog("processBlock开转");
    // float volume{0.0f};
    // for (int i = 0; i < buffer.getNumSamples(); i++) {
    //     volume += buffer.getReadPointer(0)[i];
    // }
    // Utils::writeEmergencyLog(
    //     "processBlock中的第一通道的音量平均值为" +
    //     std::to_string((volume / static_cast<float>(buffer.getNumSamples())))
    // );
}

void GodProcessor::timerCallback() {
    double currentSeconds = currentPlaySamples.load() / mSampleRate;
    juce::var obj{new juce::DynamicObject()};
    obj.getDynamicObject()->setProperty(AudioDefs::currentPTS, currentSeconds);
    auto msg = juce::JSON::toString(obj).toStdString();
    mAudioProcessWorker->sender->sendMessage(msg);
}

GodProcessor::~GodProcessor() {
    stopTimer(); // 300ms的计时器不需要暂停吧
}

void GodProcessor::setStateInformation(const void* data, int sizeInBytes) {}
void GodProcessor::getStateInformation(juce::MemoryBlock& destData) {}

bool GodProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    auto outSet = layouts.getMainOutputChannelSet();
    // 只要输出不是禁用状态，且通道数在 1~256 之间，就允许声卡进行绑定
    return !outSet.isDisabled() && outSet.size() > 0 && outSet.size() <= 256;
}