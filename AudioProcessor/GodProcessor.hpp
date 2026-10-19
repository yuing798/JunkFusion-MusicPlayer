#pragma once

#include "./BeforeCrossFadeDSPModule.hpp"
#include "CrossFadeArea.hpp"
#include "DeviceManager.hpp"
#include "Model/PlayInfo.hpp"
#include "PlayCount.hpp"
#include "PlayPauseControl.hpp"
#include "ProgressBarWatch.hpp"
#include "SmoothSliderControl.hpp"
#include "SystemAudioControl.hpp"
#include "Utils/constants.h"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <spdlog/logger.h>
#include <string>

class GodProcessor : public juce::AudioProcessor {
private:
    AudioProcessWorker mWorker;
    OscReceiver mOscReceiver;

    float mSampleRate{44100.0f};
    int mNumChannels{2};

    PlayInfo mPlayInfo;

    std::unique_ptr<SystemAudioControl> mSystemAudioControl; // 系统级音频控制

    ProgressBarWatch mProgressBar;
    PlayCount mPlayCount;
    PlayPauseControl mPlayPauseControl;
    CrossFadeArea mCrossFade;
    SmoothSliderControl mSmooth;

    struct BeforeCrossFadeDSPModuleAndBuffer {
        juce::AudioBuffer<float> buffer;
        BeforeCrossFadeDSPModule module;

        BeforeCrossFadeDSPModuleAndBuffer(AudioProcessWorker& worker, SmoothSliderControl& smooth)
            : module(worker, smooth) {}
    };
    std::array<BeforeCrossFadeDSPModuleAndBuffer, 2> mDoubleDecks;

    // DeviceManager mDeviceManager; // 设备管理要在所有音频处理子类之前就析构

    std::unique_ptr<DeviceManager> mDeviceManager;

    /**
     * @brief 播放上一首歌曲或者下一首歌曲的回调请求
     *
     * @param nextOrPrevious true为下一首，false为上一首
     */
    void playNextOrPreviousSong(bool nextOrPrevious);

    /**
     * @brief Set the Ordinary Play
     * object,设置普通的播放状态，也就是不走交叉淡化那一条路径，只是普通的淡入处理
     *
     */
    void continuePlay(double targetSeconds);
    void pausePlay();
    void play(PlayInfo info, double targetSeconds, bool isSeek, bool isNewSong);

public:
    GodProcessor(
        std::string pushPullPort,
        std::string pubSubPort,
        juce::File cacheDir,
        int oscPort
    );
    ~GodProcessor();
    const juce::String getName() const override { return "JunkFusion"; }
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {
        juce::ignoreUnused(index);
        return;
    }
    const juce::String getProgramName(int index) override {
        juce::ignoreUnused(index);
        return "JunkFusion";
    }
    void changeProgramName(int index, const juce::String& newName) override {
        juce::ignoreUnused(index);
        juce::ignoreUnused(newName);
        return;
    }
    void setStateInformation(const void* data, int sizeInBytes) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    DONT_COPY_AND_MOVE(GodProcessor)
};