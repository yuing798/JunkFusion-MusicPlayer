#pragma once

#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <functional>
#include <memory>
#include <string>
// 发送普通数据给音频进程
class AudioProcessPusher : public juce::Thread {
public:
    AudioProcessPusher(zmq::context_t& context, juce::WaitableEvent& e);
    ~AudioProcessPusher();
    void run() override;
    std::string getPort() noexcept { return mPushPullPort; }

    // 给其他线程调用的公共接口
    void sendMessage(const std::string& msg);

private:
    zmq::context_t& mContext;
    std::string mPushPullPort;
    juce::WaitableEvent& portInitOK;

    std::mutex queueMutex;
    std::queue<std::string> messageQueue;
    juce::WaitableEvent wakeUpEvent; // 发送消息需要的锁队列模式
};

// 接收音频进程传过来的紧急事件和广播状态
class AudioProcessSuber : public juce::Thread {
private:
    zmq::context_t& mContext;
    std::string mPubSubPort;
    juce::WaitableEvent& portInitOK;

public:
    AudioProcessSuber(zmq::context_t& context, juce::WaitableEvent& e);
    ~AudioProcessSuber();
    void run();
    std::string getPort() noexcept { return mPubSubPort; }
};
class AudioProcessCoordinator {
public:
    AudioProcessCoordinator();
    ~AudioProcessCoordinator();
    bool start(const juce::File& exeFile, int oscPort, juce::File cacheDir);

    void stop();
    std::unique_ptr<AudioProcessPusher> mAudioProcessPusher;
    std::unique_ptr<AudioProcessSuber> mAudioProcessSuber;

private:
    zmq::context_t mContext{1};
    juce::WaitableEvent pushInitOK;
    juce::WaitableEvent subInitOK;
    juce::ChildProcess mChildProcess; // 负责拉起并监控后端进程

    bool mRunning{false};
};
