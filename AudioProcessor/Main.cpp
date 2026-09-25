
#include "GodProcessor.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/constants.h"
#include "Utils/otherUtils.hpp"
#include "juce_events/juce_events.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

int main(int argc, char* argv[]) {
    OtherUtils::writeEmergencyLog("开始初始化音频进程1");
    // ── 1. 从命令行解析协调者传来的 zmq 地址 ──
    juce::StringArray args{argv, argc};
    for (int i = 0; i < args.size(); i++) {
        if (args[i] == AudioMacro::cacheDir) {
            auto cacheDir = juce::File{args[i + 1]};
            OtherUtils::initAudioLogger(cacheDir);
            break;
        }
    }

    // juce::String commandLine = args.joinIntoString(" ");
    // OtherUtils::writeEmergencyLog(commandLine.toRawUTF8());

    {

        juce::ScopedJuceInitialiser_GUI juceInitialiser; // 消息队列初始化

        GodProcessor processor{
            std::move(args)
        }; // 音频线程是唯一的核心，其他所有的类都服务于音频线程

        auto logger{spdlog::get(LogAudioID)};

        logger->debug("音频进程开始阻塞");
        // OtherUtils::writeEmergencyLog("音频进程开始阻塞");

        juce::MessageManager::getInstance()->runDispatchLoop();

        logger->debug("音频进程准备销毁");
        // OtherUtils::writeEmergencyLog("音频进程要死了!");
    }
    spdlog::get(LogAudioID)
        ->debug("------------------------------------------------------------------------");
    spdlog::shutdown();

    return 0;
}
