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

    /** @brief 重建整条全局效果器链（参数是 {"bypassAll":..,"slots":[..]}） */
    std::function<void(const Yvar& config)> onPluginChainConfig;

    /** @brief 整链旁通开关 */
    std::function<void(bool bypassAll)> onPluginChainBypassAll;

    /** @brief 单个插槽旁通开关 */
    std::function<void(int slot, bool bypass)> onPluginChainSlotBypass;

    /** @brief 打开某个插槽的插件编辑器窗口 */
    std::function<void(int slot)> onPluginChainOpenEditor;

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

    /// 心跳间隔：Coordinator 那边 3 秒收不到就判进程挂死（见 audioMacro::heartbeat）
    static constexpr int kHeartbeatIntervalMs = 500;

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

    /**
     * @brief 启动收发两个线程（**必须等所有回调接好之后再调**）
     *
     * 为什么不在构造函数里直接 startThread()：
     *   构造时线程就绪，而回调是在 GodProcessor 构造函数体里才赋值的。
     *   Coordinator 拉起进程后立刻发 `spectrumShm`（建共享内存那条），
     *   一旦落进"线程已在收消息、回调还是 nullptr"这个窗口，puller 里的
     *   `if (onSpectrumAttach)` 就把它**静默丢掉**了。attach 是一次性的，
     *   丢了整场都没有频谱。实测 19 次启动丢了 5 次。
     *   （启动时的 `play` 自动续播走的是同一条路，同样会丢。）
     *
     * 现在由 GodProcessor 在构造函数末尾调用，此时 mSpectrum 等成员都已构造完、
     * 所有回调都已赋值；在此之前 sendMessage 只是入队，不会丢。
     */
    void start();

    // 这两个本来就是完全供给外界调用的，放在这里只不过是用来集中管理,所以直接public就行
    std::unique_ptr<AudioProcessorPuller> receiver;
    std::unique_ptr<AudioProcessorPuber> sender;
};
