#pragma once

#include "AudioPreProcess.hpp"
#include "AudioRingBuffer.hpp"
#include "DeviceManager.hpp"
#include "constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "processSchedule/AudioProcessWorker.hpp"
#include "processSchedule/OscReceiver.hpp"
#include "zmq.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <spdlog/logger.h>

class GodProcessor : public juce::AudioProcessor {
private:
    std::unique_ptr<OscReceiver> mOscReceiver;
    std::unique_ptr<DeviceManager> mDeviceManager;
    std::unique_ptr<AudioProcessWorker> mAudioProcessWorker;
    juce::File mCacheDir;

    AudioPreProcess mPreProcess; // 音频预处理

    double mSampleRate{44100.0};
    int mNumChannels{2};
    std::atomic<int> currentPlaySamples{0}; // 该歌曲已经播放的采样点总数

public:
    GodProcessor(juce::StringArray initArgs);
    ~GodProcessor() = default;
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