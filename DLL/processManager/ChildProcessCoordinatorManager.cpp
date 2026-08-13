#include "./ChildProcessCoordinatorManager.h"
#include "constants.h"
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>

ChildProcessCoordinatorManager::ChildProcessCoordinatorManager(
    juce::File exeFile,
    juce::String workerId,
    int timeOutMs,
    int maxRestartTimes

)
    : mExeFile(exeFile), mWorkerId(workerId), mTimeoutMs(timeOutMs),
      mMaxRestartsTimes(maxRestartTimes) {}

void ChildProcessCoordinatorManager::handleConnectionLost() {
    isRunning = false;

    // 如果还未达到最大重启次数，尝试自动重启
    if (restartCount < mMaxRestartsTimes) {
        ++restartCount;
        // 短暂延时，避免快速循环崩溃
        juce::Thread::sleep(500);
        start(); // 递归调用自己尝试重启
    } else {
        // 超过次数，可以通知外部（例如调用一个虚函数让子类处理，比如弹窗）
        // onMaxRestartsExceeded();
        restartCount = 0; // 重置以便下次手动启动
    }
}

bool ChildProcessCoordinatorManager::start() {
    if (isRunning) return true;

    if (launchProcess()) {
        isRunning = true;
        restartCount = 0; // 启动成功后重置崩溃计数
        return true;
    }
    return false;
}

void ChildProcessCoordinatorManager::stop() {
    if (!isRunning) return;

    killWorkerProcess();
    isRunning = false;
}

bool ChildProcessCoordinatorManager::launchProcess() {
    // 调用基类的 launchSlaveProcess，超时参数可以统一设置，或从子类获取
    if (launchWorkerProcess(
            mExeFile,
            mWorkerId,
            mTimeoutMs // 超时 5 秒结束
        )) {
        auto logger{spdlog::get(LogDllID)};
        logger->info("{}进程开始启动", mExeFile.getFileName().toStdString());
        getInitArgs()->setProperty(AudioDefs::connectSingal, "");

        juce::String jsonStr = juce::JSON::toString(getInitArgs().get());

        // 将字符串转为 MemoryBlock（二进制块）并发送给子进程
        juce::MemoryBlock mb;
        mb.append(jsonStr.toRawUTF8(), jsonStr.getNumBytesAsUTF8());
        sendMessageToWorker(mb);

        return true;
    } else {
        auto logger{spdlog::get(LogDllID)};
        logger->critical("启动{}进程失败", mExeFile.getFileName().toStdString());
        return false;
    }
}

// bool YChildProcessCoordinator::startBackend(
//     const juce::String& backendExePath,
//     const juce::String& cacheDir
// ) {
//     juce::File exeFile(backendExePath);

//     if (launchWorkerProcess(exeFile, "JunkFusionBackend", 3000)) {
//         auto logger{spdlog::get(LogDllID)};
//         logger->info("JunkFusionBackend进程开始启动");

//         juce::DynamicObject::Ptr obj{new juce::DynamicObject()};
//         obj->setProperty("cacheDir", cacheDir);

//         juce::String jsonStr = juce::JSON::toString(obj.get());

//         // 将字符串转为 MemoryBlock（二进制块）并发送给子进程
//         juce::MemoryBlock mb;
//         mb.append(jsonStr.toRawUTF8(), jsonStr.getNumBytesAsUTF8());
//         sendMessageToWorker(mb);

//         return true;
//     } else {
//         auto logger{spdlog::get(LogDllID)};
//         logger->critical("启动JunkFusionBackend进程失败");
//     }
// }

// void YChildProcessCoordinator::stopBackend() {
//     // 先发一条"退出"指令给子进程，让它自己清理内存
//     // juce::String quitMsg = "{\"type\": \"quit\"}";
//     // juce::MemoryBlock mb;
//     // mb.append(quitMsg.toRawUTF8(), quitMsg.getNumBytesAsUTF8());
//     // sendMessageToSlave(mb);

//     // 强制切断连接并杀死子进程进程树
//     killWorkerProcess();
// }

// void YChildProcessCoordinator::handleMessageFromWorker(const juce::MemoryBlock& mb) {}

// void YChildProcessCoordinator::handleConnectionLost() {}