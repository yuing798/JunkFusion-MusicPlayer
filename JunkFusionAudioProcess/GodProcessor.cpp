#include "./GodProcessor.hpp"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"

GodProcessor::GodProcessor(juce::File cacheDir)
    : decoderRingBuffer(1000), decoder(decoderRingBuffer), mCacheDir(cacheDir) {}

void GodProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
    juce::AudioChannelSet outputLayout = getChannelLayoutOfBus(false, 0);
    decoder.prepareToPlay(outputLayout, sampleRate);
}

void GodProcessor::releaseResources() {}
void GodProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {}
void GodProcessor::setStateInformation(const void* data, int sizeInBytes) {}
void GodProcessor::getStateInformation(juce::MemoryBlock& destData) {}