// ==============================================================================
// mainUI.cpp — Qt 前端应用程序入口
// ==============================================================================
//
// 这个文件是 JunkFusionUI.exe 的 main() 函数所在位置。
// 它负责：
//   1. 初始化 Qt GUI 应用程序
//   2. 拉起 JUCE 后端进程（JunkFusionBackend.exe）
//   3. 加载并显示 QML 界面
//   4. 关闭 Qt 窗口时同步终止 JUCE 后端
//
// Qt 程序启动流程（对比传统的 C++ main 函数）：
//
//   传统 C++:  main() → 创建窗口 → 消息循环 → main() 返回
//   Qt:        main() → QApplication → QML引擎 → app.exec() → main() 返回
//                                        ↑
//                                 exec() 内部就是 Qt 的事件循环
//                                 （类似 Windows 的 GetMessage/DispatchMessage）
//                                 它阻塞在这里，不断处理用户输入、定时器、
//                                 网络事件、信号槽调用等，直到窗口关闭
//
// 关键类的职责：
//   QGuiApplication  → 管理 GUI 程序的生命周期、命令行参数、全局设置
//   QQmlApplicationEngine → 加载 .qml 文件，创建 UI 控件树，连接 C++/QML
//   JuceProcessManager → 管理 JunkFusionBackend.exe 子进程
// ==============================================================================

#include "bridge/juceProcessorManager.hpp"
#include <QDir>
#include <QGuiApplication>       // Qt GUI 应用程序基类
#include <QIcon>                 // 应用图标
#include <QQmlApplicationEngine> // QML 引擎（加载 .qml 文件并渲染 UI）
#include <QQmlContext>           // QML 上下文（用于向 QML 注入 C++ 对象）
#include <QSettings>
#include <QStandardPaths>

int main(int argc, char* argv[]) {
    // ════════════════════════════════════════════════════════════════
    // 第 1 步：创建 Qt 应用程序对象
    // ════════════════════════════════════════════════════════════════
    //
    // QGuiApplication 是 Qt 中"非 Widget 类 GUI 程序"的基类。
    // 它的父子关系：
    //   QCoreApplication  ← 最底层的事件循环 + 命令行处理
    //       ↑
    //   QGuiApplication   ← 加了窗口系统集成（OpenGL/Vulkan、剪贴板、拖放）
    //       ↑
    //   QApplication      ← 再加 Widget 相关（.ui 文件、QPushButton 等）
    //
    // 因为我们是纯 QML（没有 QWidget），所以用 QGuiApplication 就够了。
    // argc/argv 传入命令行参数（Qt 会解析 --qmljsdebugger 等自己的参数）
    QGuiApplication app(argc, argv);

    // ── 设置应用程序元信息（显示在任务管理器/关于窗口中） ──
    app.setApplicationName("JunkFusion"); // 程序名
    app.setApplicationVersion("0.0.2");   // 版本号
    app.setOrganizationName("YusekX");    // 组织名（影响 QSettings 存储路径）

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir;
    if (!dir.exists(configDir)) {
        dir.mkpath(configDir);
    }
    QString settingsPath = configDir + "/forntendSettings.ini";
    QSettings settings(settingsPath, QSettings::IniFormat);

    // ════════════════════════════════════════════════════════════════
    // 第 2 步：创建 JUCE 后端进程管理器，并启动后端
    // ════════════════════════════════════════════════════════════════
    //
    // JuceProcessManager 负责 JunkFusionBackend.exe 的完整生命周期。
    // 构造时：注册了 aboutToQuit 钩子（Qt 退出前自动 kill JUCE）
    // startDaemon()：异步启动 JUCE 进程（不阻塞 UI）
    // 钩子是一种“框架留出的插槽”，允许你在程序生命周期的特定关键时刻（如启动、关闭、空闲时）插入你自己的自定义代码。
    // 这里把它定义为 app 下面的栈变量，生命周期覆盖整个 main()：
    //   - app.exec() 返回前，manager 一直存活
    //   - app.exec() 返回后，manager 析构 → m_process 析构 → JUCE 被 kill
    JuceProcessManager juceManager;
    juceManager.startDaemon(); // 启动 JUCE 音频引擎

    // ════════════════════════════════════════════════════════════════
    // 第 3 步：创建 QML 引擎并加载主界面
    // ════════════════════════════════════════════════════════════════
    //
    // QQmlApplicationEngine 是 QML 世界的"总管家"：
    // - 解析 .qml 文件 → 创建控件树 → 渲染到屏幕
    // - 管理 C++ 和 QML 之间的双向通信
    //
    QQmlApplicationEngine engine;

    // ── 向 QML 注入 C++ 对象 ──
    // rootContext():所有加载到该引擎里的 QML 文件（无论嵌套多深）都会自动继承这个根上下文里的内容
    // setContextProperty() 让 QML 代码可以直接访问 C++ 对象
    // 语法：setContextProperty("QML中的名字", 指向C++对象的指针)
    // QML 端使用方式：
    //   import QtQuick 2.15
    //   // 直接通过名字 juceBackend 访问
    //   Button { onClicked: juceBackend.stopDaemon() }
    engine.rootContext()->setContextProperty("juceBackend", &juceManager);

    // ── 加载 QML 主文件（来自 qt_add_qml_module 定义的模块） ──
    // loadFromModule() 是 Qt 6.5+ 的新 API，替代旧的 load(QUrl("qrc:/..."))
    // 参数：
    //   "JunkFusion" → CMakeLists.txt 中 qt_add_qml_module 定义的 URI
    //   "main"       → 要加载的 QML 文件名（不含 .qml 后缀），即 main.qml
    //
    // 等价于旧写法：
    //   engine.load(QUrl("qrc:/qt/qml/JunkFusion/main.qml"));
    engine.loadFromModule("JunkFusion", "main");

    // ── 检查 QML 是否加载成功 ──
    // engine.rootObjects() 返回 QML 创建的所有顶层对象（通常是一个 ApplicationWindow）
    // 如果为空，说明 QML 解析或加载失败（语法错误、文件不存在等）
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "❌ QML 界面加载失败！请检查 main.qml 是否有语法错误";
        return -1;
    }

    // ════════════════════════════════════════════════════════════════
    // 第 4 步：进入 Qt 事件循环
    // ════════════════════════════════════════════════════════════════
    //
    // app.exec() 是 Qt 程序的核心，它会：
    // 1. 渲染第一个界面帧
    // 2. 进入死循环（事件循环），不断检查是否有新事件需要处理：
    //    - 用户点击按钮 → QML 发射信号 → C++ 槽函数被调用
    //    - 定时器到期 → 执行定时器回调
    //    - JUCE 进程有输出 → QProcess 发射 readyReadStandardOutput → lambda 被执行
    //    - 用户关闭窗口 → 窗口发射 closing 信号 → 事件循环退出
    // 3. exec() 返回后，程序从 main() 退出
    //
    // 注：exec() 返回时：
    //   - QQmlApplicationEngine 析构 → QML UI 树被销毁
    //   - juceManager 析构 → QProcess 析构 → JUCE 进程被 kill（兜底）
    //   - QGuiApplication 析构 → 清理全局状态
    return app.exec();
}
