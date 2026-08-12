#pragma once

#include "AudioRingBuffer.hpp"
#include "constants.h"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
class GodProcessor : public juce::AudioProcessor {
private:
    AudioRingBuffer decoderRingBuffer;
    FFmpegDecoder decoder;
    juce::File mCacheDir;

public:
    GodProcessor(juce::File cahceDir);
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
    void setCurrentProgram(int index) override { return; }
    const juce::String getProgramName(int index) override { return "JunkFusion"; }
    void changeProgramName(int index, const juce::String& newName) override { return; }
    void setStateInformation(const void* data, int sizeInBytes) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    DONT_COPY_AND_MOVE(GodProcessor)
};