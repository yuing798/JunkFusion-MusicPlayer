#include "./GodProcessor.hpp"
#include "ffmpegDecoder.hpp"
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"

GodProcessor::GodProcessor(int oscPort)
    : decoderRingBuffer(1000), decoder(decoderRingBuffer), mOscReceiver(oscPort) {}

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