#include "./AudioProcessCoordinator.hpp"
#include "AudioDefs.hpp"
#include "AudioProcessCoordinator.hpp"
#include "ChildProcessCoordinatorManager.h"
#include "juce_core/juce_core.h"

AudioProcessCoordinator::AudioProcessCoordinator(juce::File cahceDir, juce::File exeDir)
    : ChildProcessCoordinatorManager(
          exeDir.getChildFile("JunkFusionAudioProcess.exe"),
          "JunkFusionAudioProcess",
          5000,
          3
      ),
      mCacheDirStr(cahceDir.getFullPathName()) {}

juce::DynamicObject::Ptr AudioProcessCoordinator::getInitArgs() const {
    juce::DynamicObject::Ptr obj{new juce::DynamicObject()};
    obj->setProperty(AudioDefs::cacheDir, mCacheDirStr);
    return obj;
}
