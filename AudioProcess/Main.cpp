#include "AudioDefs.hpp"
#include "AudioProcessWorker.hpp"
#include "AudioUtils.hpp"
#include "GodProcessor.hpp"
#include "juce_events/juce_events.h"
#include "otherUtils.hpp"
#include <juce_core/juce_core.h>
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>

int main(int argc, char* argv[]) {

    juce::ScopedJuceInitialiser_GUI juceInitialiser; // 消息队列初始化
    juce::String commandLine = juce::StringArray(argv, argc).joinIntoString(" ");
    Utils::writeEmergencyLog(commandLine.toRawUTF8());
    AudioProcessWorker mAudioProcessWorker;
    juce::Thread::sleep(3000);
    if (!mAudioProcessWorker.initialiseFromCommandLine(commandLine, "JunkFusionAudioProcess", 5000))
        Utils::writeEmergencyLog("从命令行中初始化失败");
    std::unique_ptr<GodProcessor> mGodProcessor;

    juce::WaitableEvent initEvent; // 用于等待初始化参数的传入
    juce::String mCacheDir;
    bool initSuccess{false};

    mAudioProcessWorker.onInit = [&mCacheDir, &initSuccess, &initEvent](juce::String cacheDir) {
        Utils::writeEmergencyLog("开始初始化参数");
        mCacheDir = std::move(cacheDir);
        initSuccess = true;
        initEvent.signal();
    };

    initEvent.wait(3000);
    if (initSuccess == false) {
        // 这里通知调度者重启服务
        Utils::writeEmergencyLog("初始化失败，准备通知调度者重启");
        return -1;
    } // 这里链接正式完成
    auto logPath = juce::File{mCacheDir}.getChildFile("log");
    if (!logPath.exists()) logPath.createDirectory();
    auto audioLogFile = logPath.getChildFile("audioProcess.log");
    AudioUtils::initAudioLogger(audioLogFile.getFullPathName().toStdString());

    auto logger{spdlog::get(AudioDefs::LogAudioId)};
    logger->debug("音频进程开始阻塞");

    juce::MessageManager::getInstance()->runDispatchLoop();

    logger->debug("音频进程准备销毁");
    Utils::writeEmergencyLog("音频进程要死了!");

    return 0;
}
