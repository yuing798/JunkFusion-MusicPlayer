#include "AudioDefs.hpp"
#include "AudioProcessWorker.hpp"
#include "AudioUtils.hpp"
#include "GodProcessor.hpp"
#include "constants.h"
#include "juce_events/juce_events.h"
#include "otherUtils.hpp"
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
    Utils::writeEmergencyLog(commandLine.toRawUTF8());
    std::string endpoint;
    juce::File cacheDir;
    int oscPort{0};
    for (int i = 0; i < args.size(); i++) {
        if (args[i] == AudioDefs::zmqEndpoint) {
            endpoint = args[i + 1].toStdString();
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
    }
    AudioUtils::initAudioLogger(cacheDir); // 开启日志

    AudioProcessWorker worker(std::move(cacheDir));
    if (!worker.initWorker(endpoint)) {
        Utils::writeEmergencyLog("zmq: 握手失败");
    }

    auto logger{spdlog::get(LogAudioID)};
    logger->debug("音频进程开始阻塞");
    Utils::writeEmergencyLog("音频进程开始阻塞");

    juce::MessageManager::getInstance()->runDispatchLoop();

    logger->debug("音频进程准备销毁");
    Utils::writeEmergencyLog("音频进程要死了!");
    spdlog::shutdown();

    return 0;
}
