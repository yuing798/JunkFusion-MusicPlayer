
#pragma once
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"

class ChildProcessCoordinatorManager : public juce::ChildProcessCoordinator {
public:
    explicit ChildProcessCoordinatorManager(
        juce::File& exeFile,
        juce::String workerId,
        int timeOutMs,
        int maxRestartTimes

    );
    ~ChildProcessCoordinatorManager() override = default;

    // ============================================================
    // 子类必须实现的接口
    // ============================================================

    /** 返回初始化参数（不含程序路径本身）这个函数不直接构造函数传参的原因是可能有动态参数传入 */
    virtual juce::DynamicObject::Ptr getArgs() const = 0;

    // 重启次数超限时的处理，由子类实现
    virtual void onMaxRestartsExceeded() = 0;

    /** 可选：处理来自子进程的自定义消息 */
    virtual void handleMessageFromWorker(const juce::MemoryBlock& mb) override {
        juce::ignoreUnused(mb);
    }

    /** 可选：连接断开时的处理（子类可以覆盖，但通常需要基类参与重启逻辑） */
    virtual void handleConnectionLost() override;

    // ============================================================
    // 公共的启动/停止逻辑（模板方法）
    // ============================================================

    /** 启动子进程（内含重试、超时处理） */
    bool start();

    /** 停止子进程 */
    void stop();

    // protected:
    //     // 访问说明符（public/protected）控制的是“调用者”，而不是“重写者”。 派生类可以重写public
    //
    //     // 函数，也可以重写 protected 函数，但调用它们的权限天差地别。
    //     /** 子类可以在启动前做一些额外检查 */
    //     virtual bool onBeforeLaunch() { return true; }

    //     /** 子类可以在连接建立后做一些初始化（如发送配置） */
    //     virtual void onConnectionEstablished() {}

private:
    juce::File mExeFile;
    juce::String mWorkerId;

    int mTimeoutMs{5000};
    int mMaxRestartsTimes = 3;
    int restartCount = 0;
    bool isRunning = false;

    // 内部实际的启动实现
    bool launchProcess();
};

// class YChildProcessCoordinator : public juce::ChildProcessCoordinator {

// private:
//     bool isRunning{false};  // 该进程是否在运行
//     int restartCount{0};    // 重启次数的计数器
//     int maxRestartTimes{3}; // 子进程崩溃后的重启次数，如果超过三次直接弹窗并强制终止应用
//     juce::String mExeName;
//     juce::DynamicObject::Ptr mArgs;

// public:
//     YChildProcessCoordinator(juce::String exeName, const juce::DynamicObject::Ptr args)
//         : mExeName(exeName), mArgs(args) {}

//     // 析构时确保子进程被销毁
//     ~YChildProcessCoordinator() override { stopBackend(); }

//     // ========================================================================
//     // 1. 启动子进程并传入参数
//     // ========================================================================
//     bool startBackend();

//     // ========================================================================
//     // 2. 主动关闭子进程
//     // ========================================================================
//     void stopBackend();

//     // ========================================================================
//     // 3. 监听来自子进程的消息 (回调函数)
//     // ========================================================================
//     void handleMessageFromWorker(const juce::MemoryBlock& mb) override;

//     // ========================================================================
//     // 4. 监听子进程崩溃/断开 (回调函数)
//     // ========================================================================
//     void handleConnectionLost() override;
// };