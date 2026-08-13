#pragma once

#include "ChildProcessCoordinatorManager.h"
#include "juce_core/juce_core.h"
class AudioProcessCoordinator : public ChildProcessCoordinatorManager {
private:
    juce::String mCacheDirStr;

public:
    AudioProcessCoordinator(juce::File cacheDir, juce::File exeDir);
    juce::DynamicObject::Ptr getInitArgs() const override;
};