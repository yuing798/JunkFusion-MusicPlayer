#pragma once

#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include <functional>
#include <string>
class AudioProcessWorker : public juce::ChildProcessWorker {

public:
    AudioProcessWorker() = default;
    void handleMessageFromCoordinator(const juce::MemoryBlock& mb) override;
    void handleConnectionMade() override;
    void handleConnectionLost() override;
    std::function<void(juce::String)> onInit;

    DONT_COPY_AND_MOVE(AudioProcessWorker)
};