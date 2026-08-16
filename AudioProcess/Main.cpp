#include "AudioDefs.hpp"
#include "AudioUtils.hpp"
#include "DeviceManager.hpp"
#include "GodProcessor.hpp"
#include "constants.h"
#include "juce_events/juce_events.h"
#include "otherUtils.hpp"
#include "processSchedule/AudioProcessWorker.hpp"
#include <SQLiteCpp/Database.h>
#include <juce_core/juce_core.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

int main(int argc, char* argv[]) {
    Utils::writeEmergencyLog("准备初始化音频进程");

    juce::ScopedJuceInitialiser_GUI juceInitialiser; // 消息队列初始化

    // ── 1. 从命令行解析协调者传来的 zmq 地址 ──
    juce::StringArray args{argv, argc};
    juce::String commandLine = args.joinIntoString(" ");
    // Utils::writeEmergencyLog(commandLine.toRawUTF8());
    std::string pushPullPort;
    std::string pubSubPort;
    juce::File cacheDir;
    int oscPort{0};
    for (int i = 0; i < args.size(); i++) {
        if (args[i] == AudioDefs::pushPullPort) {
            pushPullPort = args[i + 1].toStdString();
            continue;
        }
        if (args[i] == AudioDefs::cacheDir) {
            cacheDir = juce::File{args[i + 1]};
            continue;
        }
        if (args[i] == AudioDefs::oscPort) {
            oscPort = args[i + 1].getIntValue();
            continue;
        }
        if (args[i] == AudioDefs::pubSubPort) {
            pubSubPort = args[i + 1].toStdString();
        }
    }
    AudioUtils::initAudioLogger(cacheDir); // 开启日志
    juce::File configFile = cacheDir.getChildFile("JFConfig.xml");
    if (!configFile.existsAsFile()) configFile.create();
    DeviceManager mDeviceManager(std::move(configFile));
    GodProcessor mGodProcessor{oscPort}; // osc是用来操作滑块的，所以直接放在godProcessor中
    AudioProcessWorker mWorker(std::move(pushPullPort), std::move(pubSubPort));

    auto logger{spdlog::get(LogAudioID)};

    logger->debug("音频进程开始阻塞");
    Utils::writeEmergencyLog("音频进程开始阻塞");

    juce::MessageManager::getInstance()->runDispatchLoop();

    logger->debug("音频进程准备销毁");
    Utils::writeEmergencyLog("音频进程要死了!");
    spdlog::shutdown();

    return 0;
}
