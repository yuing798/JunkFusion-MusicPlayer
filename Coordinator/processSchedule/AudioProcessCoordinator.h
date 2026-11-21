#pragma once

#include "juce_core/juce_core.h"
#include "zmq.hpp"

#include <atomic>
#include <cstdint>
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

    /**
     * @brief 收到音频进程心跳时回调（在 SUB 线程上）
     *
     * 音频进程每 500ms 发一条 `{"heartbeat":{...}}`；Coordinator 的看门狗
     * 靠它判断"进程还活着"。回调必须轻（只做一次原子赋值），别在这里干活。
     */
    std::function<void()> onHeartbeat;
};

/**
 * @brief 音频进程的启动器 + **看门狗**
 *
 * ── 为什么需要看门狗（插件宿主的硬要求）────────────────────────────────
 * 插件能把音频进程直接带走：实测在工厂函数里空指针写入，进程立刻消失
 * （退出码 0xC0000005，没有异常、没有回调、没有任何机会写日志）。
 * 进程内救不了，只能由外面发现 → 重启。
 *
 * 它还挂得住：Dexed 在析构里永远不退出。这种情况进程"活着"但没有心跳，
 * 所以判定不能只看 `isRunning()`，必须有心跳超时。
 *
 * ── 判定与动作 ────────────────────────────────────────────────────────
 *   子进程不在了                → 重启
 *   子进程在，但 3s 没有心跳     → 杀掉、重启（挂死）
 *   60 秒内重启超过 3 次         → 放弃，`onAudioProcessRestarted(-1)`，
 *                                 由上层弹一个"必须手动处理"的提示
 *
 * 为什么要有次数上限：插件一加载就崩的话，不设上限就是
 * 「崩溃 → 重启 → 再崩」的死循环（host-integration.md §6.2）。
 * 上层必须在重启后**不要自动恢复上次的插件**，否则上限也救不了。
 *
 * ── 线程模型 ──────────────────────────────────────────────────────────
 *   本类继承 juce::Thread，`run()` 是看门狗循环（500ms 一轮）。
 *   `noteHeartbeat()` 从 SUB 线程调，只写一个原子变量。
 */
class AudioProcessCoordinator : public juce::Thread {
public:
    /**
     * @brief 音频进程被重启后的通知（在**看门狗线程**上调用）
     *
     * @param attempt 第几次重启（从 1 开始）；-1 = 放弃自动重启，需要用户处理
     */
    std::function<void(int)> onAudioProcessRestarted;

    AudioProcessCoordinator();
    ~AudioProcessCoordinator() override;

    bool start(const juce::File& exeFile, int oscPort, juce::File cacheDir);

    void stop();

    /** @brief 心跳到达（SUB 线程调） */
    void noteHeartbeat() noexcept;

    std::unique_ptr<AudioProcessPusher> mAudioProcessPusher;
    std::unique_ptr<AudioProcessSuber> mAudioProcessSuber;

private:
    void run() override;

    /** @brief 用上次记下的命令行拉一个新的子进程 */
    bool spawnChild();

    /** @brief 子进程丢了（退出/挂死）：限流重启 */
    void handleChildLost(const char* reason);

    /// 看门狗轮询间隔
    static constexpr int kPollIntervalMs = 500;

    /// 超过这么久没心跳就当挂死（6 个心跳周期，容忍一次卡顿）
    static constexpr int kHeartbeatTimeoutMs = 3000;

    /// 重启限流：窗口内最多几次
    static constexpr int kMaxRestartsInWindow = 3;
    static constexpr int64_t kRestartWindowMs = 60000;

    zmq::context_t mContext{1};
    juce::WaitableEvent pushInitOK;
    juce::WaitableEvent subInitOK;

    /// 每次重启都新建一个：JUCE 的 ChildProcess 复用一个对象重新 start() 的
    /// 行为没有明确保证（它内部还留着上一条进程的句柄），新建最省心
    std::unique_ptr<juce::ChildProcess> mChildProcess;

    /// 启动命令行：重启时原样复用（端口不变，ZMQ socket 是 Coordinator 持有的）
    juce::StringArray mArgs;

    std::atomic<int64_t> mLastHeartbeatMs{0};
    std::atomic<bool> mRunning{false};

    int mRestartCount{0};
    int64_t mRestartWindowStart{0};
};
