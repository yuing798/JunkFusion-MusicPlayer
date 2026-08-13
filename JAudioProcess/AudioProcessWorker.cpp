#include "./AudioProcessWorker.hpp"
#include "AudioUtils.hpp"
#include "juce_core/juce_core.h"
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

void AudioProcessWorker::handleMessageFromCoordinator(const juce::MemoryBlock& mb) {}
void AudioProcessWorker::handleConnectionLost() {}
void AudioProcessWorker::handleConnectionMade() {
    // juce::var initObj{new juce::DynamicObject()};
    // initObj.getDynamicObject()->setProperty(AudioDefs::connectSingal, "");
}