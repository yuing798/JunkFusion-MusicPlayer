#include "./GodProcessor.hpp"
#include "CrossFadeArea.hpp"
#include "DeviceManager.hpp"
#include "Macro/audioMacro.hpp"
#include "PlayCount.hpp"
#include "PlayPauseControl.hpp"
#include "ProgressBarWatch.hpp"
#include "SmoothSliderControl.hpp"
#include "SystemAudioControl.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "WindowsSystemAudioControl.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

#ifdef __linux__
    #include "SystemAudioControl/LinuxSystemAudioControl.hpp"
#elif defined(__APPLE__)
    #include "SystemAudioControl/MacosSystemAudioControl.hpp"
#endif
GodProcessor::GodProcessor(
    std::string pushPullPort,
    std::string pubSubPort,
    juce::File cacheDir,
    int oscPort
)
    : mWorker(pushPullPort, pubSubPort), mOscReceiver(oscPort), mProgressBar(mWorker),
      mPlayCount(mWorker), mSmooth(mOscReceiver, mWorker),
      mDoubleDucks{
          {BeforeCrossFadeDSPModuleAndBuffer{mWorker, mSmooth},
           BeforeCrossFadeDSPModuleAndBuffer{mWorker, mSmooth}}
      },
      mDeviceManager(cacheDir.getChildFile("JFConfig.xml")) {

    // 设置系统音频管理类
    {
#ifdef _WIN32
        mSystemAudioControl = std::make_unique<WindowsSystemAudioControl>();
#elif defined(__linux__)
        mSystemAudioControl = std::make_unique<LinuxSystemAudioControl>();
#elif defined(__APPLE__)
        mSystemAudioControl = std::make_unique<MacosSystemAudioControl>();
#endif

        mSystemAudioControl->onLog = [](SystemAudioControl::LogRank rank, std::string str) {
            auto ptr{spdlog::get(LogAudioID).get()};
            if (rank == SystemAudioControl::LogRank::Debug) {
                ptr->debug(str);
            } else if (rank == SystemAudioControl::LogRank::Info) {
                ptr->info(str);
            } else if (rank == SystemAudioControl::LogRank::Error) {
                ptr->error(str);
            }
        };

        if (mSystemAudioControl->initialize()) {
            spdlog::get(LogAudioID)->debug("系统音频同步初始化成功");
        }

        mSystemAudioControl->onPlay = [this]() {
            play(mPlayInfo, mProgressBar.getCurSeconds(), 0, 0);
            mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Play);
            juce::var obj{new juce::DynamicObject()};
            obj.getDynamicObject()->setProperty(AudioMacro::playStateSync, 1);
            mWorker.sender->sendMessage(juce::JSON::toString(obj).toStdString());
        };
        mSystemAudioControl->onPause = [this]() {
            pausePlay();
            mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Pause);
            juce::var obj{new juce::DynamicObject()};
            obj.getDynamicObject()->setProperty(AudioMacro::playStateSync, 0);
            mWorker.sender->sendMessage(juce::JSON::toString(obj).toStdString());
        };

        mSystemAudioControl->onNext = [this] { playNextOrPreviousSong(true); };
        mSystemAudioControl->onPrevious = [this] { playNextOrPreviousSong(false); };

        mSystemAudioControl->onFastForward = [this](int value) {
            auto targetSeconds{mProgressBar.getCurSeconds() + (float)value};
            if (targetSeconds >= mPlayInfo.duration) {
                playNextOrPreviousSong(true);
            } else {
                play(mPlayInfo, targetSeconds, 0, 0);
            }
        };
        mSystemAudioControl->onRewind = [this](int value) {
            auto targetSeconds{mProgressBar.getCurSeconds() - (float)(value)};
            if (targetSeconds < 0.0) {
                playNextOrPreviousSong(false);
            } else {
                play(mPlayInfo, targetSeconds, 0, 0);
            }
        };
    }

    // 停止播放，不代表现在开始完全静音
    mWorker.receiver->onPausePlay = [this]() { pausePlay(); };

    // 完全静音的切换态
    mPlayPauseControl.onPauseOver = [this]() {
        mPlayCount.pauseCount();
        mProgressBar.pauseProgressBar();

        // 这里等下要把出水口堵住并且重置前面的DSP模块
    };

    mCrossFade.onCrossFadeEnd = [this] {};

    // 播放
    mWorker.receiver->onPlay =
        [this](PlayInfo info, double targetSeconds, bool isSeek, bool isNewSong) {
            play(std::move(info), targetSeconds, isSeek, isNewSong);
        };

    for (int i = 0; i < 2; i++) {
        mDoubleDucks[i].module.onRequestPlayNextSong = [this]() {
            playNextOrPreviousSong(true);
            mPlayCount.pauseCount();
            mProgressBar.pauseProgressBar();
        };
    }

    // 初始化设备管理者
    {
        mDeviceManager.connectProcessor(this); // 设备链接必须放到构造函数的末尾，绝对不能移动
    }
}

void GodProcessor::playNextOrPreviousSong(bool nextOrPrevious) {
    juce::var obj{new juce::DynamicObject()};
    auto ptr{obj.getDynamicObject()};
    jassert(ptr);
    ptr->setProperty(AudioMacro::onPlayNextOrPreviousSong, nextOrPrevious);
    mWorker.sender->sendMessage(juce::JSON::toString(obj).toStdString());
}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    mSampleRate = (float)sampleRate;

    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    mNumChannels = outputLayout.size();
    mProgressBar.prepareToPlay(mSampleRate);
    mPlayCount.prepareToPlay(mSampleRate);
    mSmooth.prepareToPlay(mSampleRate);
    mPlayPauseControl.prepareToPlay(mSampleRate);
    mCrossFade.prepareToPlay(mSampleRate);
    for (int i = 0; i < 2; i++) {
        // 这里只需要分配最大缓冲区数目是因为这个缓冲区只需要存储交叉淡化前最后一环的数据
        mDoubleDucks[i].buffer.setSize(mNumChannels, maximumExpectedSamplesPerBlock);
    }
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused(midiMessages);
    auto finalNumSamples{buffer.getNumSamples()}; // 最终需要的缓冲区大小
    auto speedShifterNumSamples{
        static_cast<int>(finalNumSamples * mStretch.getSpeedShifterValue())
    }; // 解码-.变速之间都要根据这个缓冲区大小做计算

    mPreProcess.processBlock(mSpeedShifterBuffer, speedShifterNumSamples);
    // buffer.clear();
    mStretch.processBlock(mSpeedShifterBuffer, speedShifterNumSamples, buffer, finalNumSamples);
    // for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
    //     buffer.copyFrom(ch, 0, mSpeedShifterBuffer.getReadPointer(ch), finalNumSamples);
    // } // 这个做个测试

    mCrossFade.processBlock(
        {&mDoubleDucks[0].buffer, &mDoubleDucks[1].buffer},
        buffer,
        buffer.getNumSamples()
    ); // 交叉淡化

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        auto currentMasterVolume = mSmooth.masterVolume.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            auto ptr{buffer.getWritePointer(ch)};
            ptr[i] *= currentMasterVolume;
        }
    } // 主音量控制（不需要上下文的DSP模块建议都放到这个循环里面）
    mPlayPauseControl.processBlock(buffer, buffer.getNumSamples()); // 播放暂停
    mPlayCount.processBlock(buffer.getNumSamples());                // 播放计数
    mProgressBar.processBlock(buffer.getNumSamples());              // 进度条
}

void GodProcessor::setOrdinaryPlay(double targetSeconds) {
    if (targetSeconds == 0.0) {
        mPlayPauseControl.setPlaybackState(
            PlayPauseControl::PlayPauseState::Play,
            PlayPauseControl::BeginEndState::Begin
        );
    } else {
        mPlayPauseControl.setPlaybackState(
            PlayPauseControl::PlayPauseState::Play,
            PlayPauseControl::BeginEndState::Nor
        );
    }
}

void GodProcessor::pausePlay() {
    mSystemAudioControl->updatePlaybackState(SystemAudioControl::PlaybackState::Pause);
    mPlayPauseControl.setPlaybackState(
        PlayPauseControl::PlayPauseState::Pause,
        PlayPauseControl::BeginEndState::Nor
    );
}

void GodProcessor::play(PlayInfo info, double targetSeconds, bool isSeek, bool isNewSong) {
    // if (isCrossFade) {
    //     waitCrossFadeOver.wait(1000);
    // }
    if (isNewSong) {
        mPlayInfo = info;
        SystemAudioControl::MediaMetadata data{};
        data.title = info.title.toStdString();
        if (info.artists.has_value())
            data.artists = ConvertUtils::stringArrayToVector(info.artists.value());
        mSystemAudioControl->updateMetadata(data);

        mPlayCount.setNewSong(info.path, info.duration);
        mProgressBar.beginProgressBar((float)targetSeconds);

        if (mDoubleDucks[mCrossFade.getMainDuckIndex()].module.getDecoderRunning()) {
            // 如果是新歌且原歌正在播放，切换甲板索引,采取600ms的交叉淡化长度
            mCrossFade.setCrossFadeBegin(true);
        } else {
            // 正常的播放
            setOrdinaryPlay(targetSeconds);
        }
    } else {
        mPlayCount.continueCount();
        if (isSeek) {
            if (mDoubleDucks[mCrossFade.getMainDuckIndex()].module.getDecoderRunning()) {
                // 不是新歌但是跳转位置，且原歌曲正在播放，采取50ms交叉长度
                mCrossFade.setCrossFadeBegin(false);
            } else {
                setOrdinaryPlay(targetSeconds);
            }
        } else {
            setOrdinaryPlay(targetSeconds);
        }
    }
    mProgressBar.beginProgressBar(targetSeconds);
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