#pragma once

#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "zmq.hpp"

#include <functional>
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
    std::function<void(std::string, double targetPTS, double duration)> onPlay;
    std::function<void(void)> onPausePlay;
    std::function<void(float)> onMasterVolumeChange;

    void run() override;
};

// ==========================================
// 线程 2：专职向 UI 发送状态/异常 (PUB)
// ==========================================
class AudioProcessorPuber : public juce::Thread, public juce::Timer {
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
    void timerCallback() override;
};

// ==========================================
// 音频进程的主管类 (管理 Context 和 线程)
// ==========================================
class AudioProcessWorker {
private:
    zmq::context_t zmqContext{1};

public:
    AudioProcessWorker(std::string pushPullPort, std::string pubSubPort);

    ~AudioProcessWorker();

    // 这两个本来就是完全供给外界调用的，放在这里只不过是用来集中管理,所以直接public就行
    std::unique_ptr<AudioProcessorPuller> receiver;
    std::unique_ptr<AudioProcessorPuber> sender;
};