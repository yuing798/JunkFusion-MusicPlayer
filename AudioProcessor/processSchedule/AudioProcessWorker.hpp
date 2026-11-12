#pragma once

#include "Model/EqInfo.hpp"
#include "Model/PlayInfo.hpp"
#include "Utils/Yvar.hpp"
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
    std::function<void(PlayInfo info, float targetSeconds, bool isSeek, bool isNewSong)> onPlay;
    std::function<void(void)> onPausePlay;
    std::function<void(float)> onMasterVolumeChange;
    std::function<void(float)> onSpeedShifterChange;
    std::function<void(float)> onPitchShifterChange;

    /** @brief EQ 的 JSON 报文（结构变更 / 拖拽结束的最终值 / 预设），非 EQ 报文不会走到这里 */
    std::function<void(const EqWire::Message& message)> onEqMessage;

    /** @brief Coordinator 要求接上频谱共享内存（参数是完整名字和字节数） */
    std::function<void(const std::string& name, size_t bytes)> onSpectrumAttach;

    /** @brief 打开/关闭频谱分析 */
    std::function<void(bool enabled)> onSpectrumEnabled;

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
    zmq::context_t zmqContext{1};

public:
    AudioProcessWorker(std::string pushPullPort, std::string pubSubPort);

    ~AudioProcessWorker();

    // 这两个本来就是完全供给外界调用的，放在这里只不过是用来集中管理,所以直接public就行
    std::unique_ptr<AudioProcessorPuller> receiver;
    std::unique_ptr<AudioProcessorPuber> sender;
};
