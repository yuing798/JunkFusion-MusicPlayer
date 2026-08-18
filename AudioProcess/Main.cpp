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
    Utils::writeEmergencyLog("开始初始化音频进程");

    juce::ScopedJuceInitialiser_GUI juceInitialiser; // 消息队列初始化

    // ── 1. 从命令行解析协调者传来的 zmq 地址 ──
    juce::StringArray args{argv, argc};
    juce::String commandLine = args.joinIntoString(" ");
    Utils::writeEmergencyLog(commandLine.toRawUTF8());
    GodProcessor mGodProcessor{
        std::move(args)
    }; // 音频线程是唯一的核心，其他所有的类都服务于音频线程

    auto logger{spdlog::get(LogAudioID)};

    logger->debug("音频进程开始阻塞");
    Utils::writeEmergencyLog("音频进程开始阻塞");

    juce::MessageManager::getInstance()->runDispatchLoop();

    logger->debug("音频进程准备销毁");
    Utils::writeEmergencyLog("音频进程要死了!");
    spdlog::get(LogAudioID)
        ->debug("------------------------------------------------------------------------");
    spdlog::shutdown();

    return 0;
}
