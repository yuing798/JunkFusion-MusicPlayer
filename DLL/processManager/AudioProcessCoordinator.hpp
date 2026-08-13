#pragma once

#include "ChildProcessCoordinatorManager.h"
#include "juce_core/juce_core.h"
class AudioProcessCoordinator : public ChildProcessCoordinatorManager {
private:
    juce::String mCacheDirStr;

public:
    AudioProcessCoordinator(juce::String cacheDirStr);
    juce::DynamicObject::Ptr getInitArgs() const override;
};