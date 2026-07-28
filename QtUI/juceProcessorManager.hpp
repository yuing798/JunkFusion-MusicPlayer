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
// │          QProcess* m_process 通过 this 作为 parent                  │
// │          → main() 返回时自动析构 manager → 自动析构 m_process       │
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

class JuceProcessManager : public QObject {
    Q_OBJECT  // ← 必须！触发 MOC（元对象编译器）生成信号槽/反射代码

public:
    // explicit 防止隐式类型转换
    // parent 参数传给 QObject 基类，建立父子关系
    explicit JuceProcessManager(QObject* parent = nullptr)
        : QObject(parent)
    {
        // ── 创建 QProcess 子对象 ──
        // new QProcess(this) 中的 this 是 parent：
        // 当 JuceProcessManager 析构时，Qt 会自动 delete 这个 m_process
        m_process = new QProcess(this);

        // ── 连接信号：进程退出 → 槽函数 onProcessFinished ──
        // connect() 的 5 个参数：
        //   ① 发送者    (m_process)
        //   ② 信号      (&QProcess::finished)     — "我退出了"
        //   ③ 接收者    (this)
        //   ④ 槽函数    (&JuceProcessManager::onProcessFinished) — "好的，我来处理"
        //   ⑤ 连接类型  (省略，使用默认的 AutoConnection)
        connect(m_process, &QProcess::finished,
                this, &JuceProcessManager::onProcessFinished);

        // ── 连接信号：JUCE 有 stdout 输出 → 打印到 Qt 的调试控制台 ──
        // 这里用 lambda 作为槽函数（Qt 5+ 支持），比传统成员函数更简洁
        // readAllStandardOutput() 返回 QByteArray，
        // trimmed() 去掉首尾空白字符
        connect(m_process, &QProcess::readyReadStandardOutput,
                this, [this]() {
                    qDebug() << "[JUCE Backend]:"
                             << m_process->readAllStandardOutput().trimmed();
                });

        // ── 核心：Qt 即将退出 → 先杀掉 JUCE 进程，防止残留孤儿进程 ──
        // qApp 是全局宏，指向当前应用程序实例
        // &QCoreApplication::aboutToQuit 在事件循环结束前发射
        // 这是最后一刻的安全钩子：确保子进程在父进程退出前被清理
        connect(qApp, &QCoreApplication::aboutToQuit,
                this, &JuceProcessManager::stopDaemon);
    }

    // ── 启动 JUCE 后端进程 ──
    // QApplication::applicationDirPath() 返回当前 exe 所在目录的绝对路径
    // 因为两个 exe 都输出到 build/debug/bin/，所以在同一个目录下
    void startDaemon()
    {
        // 拼接 JunkFusionBackend.exe 的完整路径
        const QString daemonPath =
            QCoreApplication::applicationDirPath() + "/JunkFusionBackend.exe";

        qDebug() << "正在启动 JUCE 音频引擎:" << daemonPath;

        // 异步启动进程（不会阻塞 UI 线程）
        // QProcess 会在后台等待进程就绪
        m_process->start(daemonPath);

        // waitForStarted(5000) 最多等 5 秒，超时返回 false
        if (!m_process->waitForStarted(5000)) {
            qCritical() << "❌ 无法启动 JUCE 音频引擎！路径:" << daemonPath
                        << "错误:" << m_process->errorString();
        } else {
            qDebug() << "✅ JUCE 音频引擎已启动，PID:" << m_process->processId();
        }
    }

    // ── 停止 JUCE 后端进程 ──
    // 设计思路：优先优雅退出，兜底暴力 kill
    void stopDaemon()
    {
        // 先检查进程是否还在运行
        if (m_process->state() != QProcess::Running) {
            return;  // 已经退出了，无需处理
        }

        qDebug() << "正在关闭 JUCE 音频引擎...";

        // TODO: 后续实现 IPC（进程间通信）后，可以先发 JSON 指令让 JUCE 自己退出
        // 例如：sendIpcCommand(R"({"action": "quit"})");
        // 然后 waitForFinished(3000) 等 3 秒，超时再 kill

        // ── 当前实现：直接终止进程 ──
        // kill() 发送操作系统信号强制终止（Windows 上等效 TerminateProcess）
        m_process->kill();

        // waitForFinished(3000) 阻塞最多 3 秒等待进程真正结束
        if (!m_process->waitForFinished(3000)) {
            qWarning() << "⚠️ JUCE 进程未能在 3 秒内退出，可能已僵死";
        } else {
            qDebug() << "✅ JUCE 音频引擎已关闭";
        }
    }

    // ── 检查 JUCE 后端是否在运行 ──
    bool isRunning() const
    {
        return m_process->state() == QProcess::Running;
    }

private slots:
    // ════════════════════════════════════════════════════════════════
    // "slots" 是 Qt 的关键字（通过 MOC 处理），表示下面的函数
    // 可以被信号（signal）调用。
    // 普通成员函数不能被信号直接调用，必须标记为 slot。
    // ════════════════════════════════════════════════════════════════

    // ── 进程退出后的回调 ──
    // exitCode：进程的返回值（0 表示正常退出，非 0 表示异常）
    // exitStatus：
    //   QProcess::NormalExit → 进程自己调用 exit() 退出
    //   QProcess::CrashExit  → 进程崩溃（访问非法内存、被 kill 等）
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        if (exitStatus == QProcess::CrashExit) {
            qWarning() << "⚠️ JUCE 音频进程崩溃！退出码:" << exitCode;
            // TODO: 后续可以在这里加上自动重启逻辑
            // startDaemon();  // 谨慎使用，避免无限重启循环
        } else {
            qDebug() << "JUCE 音频进程正常退出，退出码:" << exitCode;
        }
    }

private:
    // QProcess* 是原始指针，但不用担心内存泄漏：
    // 构造函数中 new QProcess(this) 把 this 设为 parent，
    // Qt 的父子树机制会在 JuceProcessManager 析构时自动 delete m_process
    QProcess* m_process;
};
