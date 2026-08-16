#pragma once

#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <memory>
#include <string>

// ==========================================
// 线程 1：专职接收 UI 指令 (PULL)
// ==========================================
class AudioProcessorPuller : public juce::Thread {
private:
    zmq::context_t& context;
    std::string mPushPullPort;

public:
    AudioProcessorPuller(zmq::context_t& ctx, std::string pushPullPort);

    void run() override;
};

// ==========================================
// 线程 2：专职向 UI 发送状态/异常 (PUB)
// ==========================================
class AudioProcessorPuber : public juce::Thread {
private:
    zmq::context_t& context;
    std::mutex queueMutex;
    std::queue<std::string> messageQueue;
    juce::WaitableEvent wakeUpEvent;
    std::string mPubSubPort;

public:
    AudioProcessorPuber(zmq::context_t& ctx, std::string pubSubPort);

    // 给其他线程（如音频处理线程）调用的公共接口
    void sendMessage(const std::string& msg);

    void run() override;
};

// ==========================================
// 音频进程的主管类 (管理 Context 和 线程)
// ==========================================
class AudioProcessWorker {
private:
    // 整个进程只需要 1 个 Context (1个后台I/O线程足够了)
    zmq::context_t zmqContext{1};

    std::unique_ptr<AudioProcessorPuller> receiver;
    std::unique_ptr<AudioProcessorPuber> sender;

public:
    AudioProcessWorker(std::string pushPullPort, std::string pubSubPort);

    ~AudioProcessWorker();

    // 暴露给音频引擎调用的接口
    void triggerEmergency(const std::string& error) { sender->sendMessage("[EMERG]" + error); }
};