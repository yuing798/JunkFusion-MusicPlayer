// ==============================================================================
// JuceProcessManager.h — JUCE 后端进程生命周期管理器
// ==============================================================================
//
// 职责：
//   1. 在 Qt 启动时拉起 JunkFusionBackend.exe（JUCE 音频引擎）
//   2. 在 Qt 关闭时安全终止 JUCE 进程
//   3. 监听 JUCE 进程的输出（stdout → qDebug）和状态（崩溃→告警）
//
// Qt 核心概念速查（按阅读顺序）：
//
// ┌─ QObject ─────────────────────────────────────────────────────────┐
// │ Qt 所有对象的"老祖宗"。继承它就获得了三个核心超能力：                │
// │                                                                     │
// │ ① 父子树内存管理：父对象析构时自动 delete 所有子对象                 │
// │    示例：JuceProcessManager 在 main() 栈上创建                      │
// │          QProcess* mProcess 通过 this 作为 parent                  │
// │          → main() 返回时自动析构 manager → 自动析构 mProcess       │
// │          全程无需手写 delete。                                       │
// │                                                                     │
// │ ② 信号与槽（Signal & Slot）：对象间解耦通信                         │
// │    信号：当某件事发生时会自动发射（emit），不需要知道谁来接收         │
// │    槽：被信号调用的普通成员函数，被调用时执行响应逻辑                 │
// │    connect(发送者, 信号, 接收者, 槽) 负责把两者绑定起来              │
// │                                                                     │
// │ ③ 属性系统（Q_PROPERTY）：可以给类定义动态属性，暴露给 QML 使用     │
// └────────────────────────────────────────────────────────────────────┘
//
// ┌─ Q_OBJECT 宏 ──────────────────────────────────────────────────────┐
// │ 写在类声明最前面的"魔咒"。它触发 Qt 的元对象编译器（MOC），          │
// │ 在编译前自动为这个类生成反射代码。                                   │
// │ 没有它 → 信号槽不工作、QML 看不到属性、qobject_cast 失败。          │
// └────────────────────────────────────────────────────────────────────┘
//
// ┌─ QProcess ─────────────────────────────────────────────────────────┐
// │ 用来启动和管理外部程序的 Qt 类。相当于 C++ 的 std::system()         │
// │ 但可以异步监听输出、等待完成、获取退出码。                           │
// │ 关键方法：                                                           │
// │   start(exe路径)       → 异步启动外部程序（不阻塞当前线程）         │
// │   kill()               → 强制终止外部进程                           │
// │   waitForFinished()    → 阻塞等待进程退出                            │
// │ 关键信号：                                                           │
// │   finished(code,status) → 进程退出时自动发射                         │
// │   readyReadStandardOutput → 进程有新 stdout 输出时发射               │
// └────────────────────────────────────────────────────────────────────┘
//
// ┌─ qApp 全局指针 ────────────────────────────────────────────────────┐
// │ qApp 是一个全局宏，指向当前的 QCoreApplication（或其子类）实例。    │
// │ 等价于 QCoreApplication::instance()。                                │
// │ 这里用它来连接 aboutToQuit 信号：程序即将退出时的最后通知。         │
// └────────────────────────────────────────────────────────────────────┘

#pragma once
#include <QCoreApplication>
#include <QDebug>
#include <QObject>
#include <QProcess>
#include <qprocess.h>

class JuceProcessManager : public QObject {
Q_OBJECT // ← 必须！触发 MOC（元对象编译器）生成信号槽/反射代码

    public :
    // explicit 防止隐式类型转换
    // parent 参数传给 QObject 基类，建立父子关系
    explicit JuceProcessManager(QObject* parent = nullptr);

    // ── 启动 JUCE 后端进程 ──
    // QApplication::applicationDirPath() 返回当前 exe 所在目录的绝对路径
    // 因为两个 exe 都输出到 build/debug/bin/，所以在同一个目录下
    void startDaemon();

    // ── 检查 JUCE 后端是否在运行 ──
    bool isRunning() const { return mProcess->state() == QProcess::Running; }

private:
    // QProcess* 是原始指针，但不用担心内存泄漏：
    // 构造函数中 new QProcess(this) 把 this 设为 parent，
    // Qt 的父子树机制会在 JuceProcessManager 析构时自动 delete mProcess
    QProcess* mProcess;
};
