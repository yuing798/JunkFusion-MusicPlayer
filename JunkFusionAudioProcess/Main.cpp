#include "AudioProcessWorker.hpp"
#include "GodProcessor.hpp"
#include "juce_events/juce_events.h"
#include <juce_core/juce_core.h>
#include <memory>

int main(int argc, char* argv[]) {

    juce::String commandLine = juce::StringArray(argv, argc).joinIntoString(" ");
    AudioProcessWorker mAduioProcessWorker;
    mAduioProcessWorker.initialiseFromCommandLine(commandLine, "JunkFusionAudioProcess", 3000);
    juce::ScopedJuceInitialiser_GUI juceInitialiser; // 消息队列初始化
    std::unique_ptr<GodProcessor> mGodProcessor;
    auto argsArray = juce::JUCEApplicationBase::getCommandLineParameterArray();
    for (int i = 0; i < argsArray.size(); i++) {
        if (argsArray[i] == "--cacheDir") {
            mGodProcessor = std::make_unique<GodProcessor>(juce::File{argsArray[i + 1]});
        }
    }

    return 0;
}
