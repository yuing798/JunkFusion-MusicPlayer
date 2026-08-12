#pragma once

#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
class AudioProcessWorker : public juce::ChildProcessWorker {
public:
    AudioProcessWorker() = default;
    void handleMessageFromCoordinator(const juce::MemoryBlock& mb) override;
    void handleConnectionMade() override;
    void handleConnectionLost() override;
};