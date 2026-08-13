#include "AudioProcessWorker.hpp"
#include "AudioUtils.hpp"
#include "GodProcessor.hpp"
#include "juce_events/juce_events.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <spdlog/logger.h>

int main(int argc, char* argv[]) {

    juce::String commandLine = juce::StringArray(argv, argc).joinIntoString(" ");
    AudioProcessWorker mAduioProcessWorker;
    mAduioProcessWorker.initialiseFromCommandLine(commandLine, "JunkFusionAudioProcess", 3000);
    juce::ScopedJuceInitialiser_GUI juceInitialiser; // 消息队列初始化
    std::unique_ptr<GodProcessor> mGodProcessor;
    std::shared_ptr<spdlog::logger> audioLogger;
    AudioUtils::initAudioLogger(mGodProcessor.)

        return 0;
}
