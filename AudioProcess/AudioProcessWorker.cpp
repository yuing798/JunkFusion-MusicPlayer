#include "./AudioProcessWorker.hpp"
#include "AudioDefs.hpp"
#include "AudioUtils.hpp"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

void AudioProcessWorker::handleMessageFromCoordinator(const juce::MemoryBlock& mb) {
    Utils::writeEmergencyLog("收到握手消息");
    auto obj{Utils::mb2object(mb)};
    if (obj->hasProperty(AudioDefs::connectSingal)) {
        if (onInit) onInit(obj->getProperty(AudioDefs::cacheDir).toString());
        return;
    }
}
void AudioProcessWorker::handleConnectionLost() {}
void AudioProcessWorker::handleConnectionMade() {
    // juce::var initObj{new juce::DynamicObject()};
    // initObj.getDynamicObject()->setProperty(AudioDefs::connectSingal, "");
    Utils::writeEmergencyLog("日");
}