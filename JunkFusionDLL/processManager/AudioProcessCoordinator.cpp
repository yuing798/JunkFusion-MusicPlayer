#include "./AudioProcessCoordinator.hpp"
#include "AudioProcessCoordinator.hpp"
#include "ChildProcessCoordinatorManager.h"
#include "juce_core/juce_core.h"

AudioProcessCoordinator::AudioProcessCoordinator(juce::String cahceDir)
    : ChildProcessCoordinatorManager(
          juce::File{"JunkFusionAudioProcess.exe"},
          "JunkFusionAudioProcess",
          5000,
          3
      ),
      mCacheDirStr(cahceDir) {}

juce::DynamicObject::Ptr AudioProcessCoordinator::getArgs() const {
    juce::DynamicObject::Ptr obj{new juce::DynamicObject()};
    obj->setProperty("cacheDir", mCacheDirStr);
    return obj;
}
