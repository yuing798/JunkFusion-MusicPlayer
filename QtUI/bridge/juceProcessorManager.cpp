#include "./juceProcessorManager.hpp"
#include <qobject.h>

JuceProcessManager::JuceProcessManager(QObject* parent) : QObject(parent) {
    // ── 创建 QProcess 子对象 ──
    // new QProcess(this) 中的 this 是 parent：
    // 当 JuceProcessManager 析构时，Qt 会自动 delete 这个 mProcess
    mProcess = new QProcess(this);
    // QProcess 是 Qt 提供的一个用于启动和管理外部操作系统进程的类。它允许你在 Qt
    // 应用程序内部，把另一个独立的程序（比如你的
    // JuceBackend.exe）当作“子任务”来运行、监控和控制。

    // ── 连接信号：进程退出 → 槽函数 onProcessFinished ──
    // connect() 的 5 个参数：
    //   ① 发送者    (mProcess)
    //   ② 信号      (&QProcess::finished)     — "我退出了"
    //   ③ 接收者    (this)
    //   ④ 槽函数    (&JuceProcessManager::onProcessFinished) — "好的，我来处理"
    //   ⑤ 连接类型  (省略，使用默认的 AutoConnection)
    connect(mProcess, &QProcess::finished, this, [](int exitCode, QProcess::ExitStatus status) {
        if (status == QProcess::CrashExit) {
            qWarning() << "⚠️ JUCE 音频进程崩溃！退出码:" << exitCode;
            // TODO: 后续可以在这里加上自动重启逻辑
            // startDaemon();  // 谨慎使用，避免无限重启循环
        } else {
            qDebug() << "JUCE 音频进程正常退出，退出码:" << exitCode;
        }
    });
    // connect的第三个参数的作用：
    // 作用一：生命周期管理（自动断开连接，防止野指针崩溃）
    //  这是第三个参数最核心的作用。它告诉 Qt：“当这个对象被销毁时，请自动帮我断开这条连接。”
    // connect 没有指定第 5 个参数（连接类型），所以默认使用 Qt::AutoConnection。此时：
    // 当第三个参数是 this（且 this 在主线程）：
    // Qt 会检测到 m_process（发送者）和 this（接收者上下文）在同一个线程（都是主线程）。那么
    // Lambda 会被直接同步执行，不需要经过事件循环。
    // 当第三个参数是 nullptr：
    // Qt 的规则是：如果接收者上下文是 nullptr，Lambda
    // 会在发送者（m_process）所在的线程执行。QProcess 通常在主线程，所以看起来区别不大。

    // ── 连接信号：JUCE 有 stdout 输出 → 打印到 Qt 的调试控制台 ──
    // 这里用 lambda 作为槽函数（Qt 5+ 支持），比传统成员函数更简洁
    // readAllStandardOutput() 返回 QByteArray，
    // trimmed() 去掉首尾空白字符
    connect(mProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        qDebug() << "[JUCE Backend]:" << mProcess->readAllStandardOutput().trimmed();
    });

    // ── 核心：Qt 即将退出 → 先杀掉 JUCE 进程，防止残留孤儿进程 ──
    // qApp 是全局宏，指向当前应用程序实例
    // &QCoreApplication::aboutToQuit 在事件循环结束前发射
    // 这是最后一刻的安全钩子：确保子进程在父进程退出前被清理
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        // 先检查进程是否还在运行
        if (mProcess->state() != QProcess::Running) {
            return; // 已经退出了，无需处理
        }

        qDebug() << "正在关闭 JUCE 音频引擎...";

        // TODO: 后续实现 IPC（进程间通信）后，可以先发 JSON 指令让 JUCE 自己退出
        // 例如：sendIpcCommand(R"({"action": "quit"})");
        // 然后 waitForFinished(3000) 等 3 秒，超时再 kill

        // ── 当前实现：直接终止进程 ──
        // kill() 发送操作系统信号强制终止（Windows 上等效 TerminateProcess）
        mProcess->kill();

        // waitForFinished(3000) 阻塞最多 3 秒等待进程真正结束
        if (!mProcess->waitForFinished(3000)) {
            qWarning() << "⚠️ JUCE 进程未能在 3 秒内退出，可能已僵死";
        } else {
            qDebug() << "✅ JUCE 音频引擎已关闭";
        }
    });
}

void JuceProcessManager::startDaemon() {
    // 拼接 JunkFusionBackend.exe 的完整路径
    const QString daemonPath = QCoreApplication::applicationDirPath() + "/JunkFusionBackend.exe";

    qDebug() << "正在启动 JUCE 音频引擎:" << daemonPath;

    // 异步启动进程（不会阻塞 UI 线程）
    // QProcess 会在后台等待进程就绪
    mProcess->start(daemonPath);

    // waitForStarted(5000) 最多等 5 秒，超时返回 false
    if (!mProcess->waitForStarted(5000)) {
        qCritical() << "❌ 无法启动 JUCE 音频引擎！路径:" << daemonPath
                    << "错误:" << mProcess->errorString();
    } else {
        qDebug() << "✅ JUCE 音频引擎已启动，PID:" << mProcess->processId();
    }
}