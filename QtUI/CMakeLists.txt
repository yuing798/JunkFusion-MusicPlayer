# ==============================================================================
# Qt 前端 — 独立 CMake 项目
# JunkFusion.exe —— 负责显示 QML 界面并管理 JUCE 后端进程
#
# 在 Qt Creator 中打开 QtUI/ 目录即可，与后端项目完全分离。
# ==============================================================================
cmake_minimum_required(VERSION 3.22)

project(JunkFusionUI VERSION 0.0.2)

# find_package 是 CMake 的"找包"命令
# REQUIRED 意味着找不到这个包就直接报错停止配置（而不是默默跳过）
# COMPONENTS 列出我们需要 Qt 的哪些模块：
#   Core    → 最基本的非 GUI 功能（信号槽、事件循环、QString、QObject 等）
#   Quick   → QML 引擎 + 2D 渲染器（包含 QQmlApplicationEngine、QQuickView）
#   Qml     → QML 语言运行时（解析 .qml 文件、类型系统、JS 引擎）
find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Quick
    Qml
)

# qt_standard_project_setup() 是 Qt 6.3+ 引入的一键初始化函数
# 它会自动做三件事：
# 1. 启用 AUTOMOC（扫描头文件中带 Q_OBJECT 宏的类，自动生成元对象代码）
# 2. 启用 AUTORCC（自动编译 .qrc 资源文件）
# 3. 启用 AUTOUIC（自动编译 .ui 文件，这与 QWidgets 相关，此处不影响）
# 我们之前在 CMakePresets.json 中手动设置了 AUTOMOC/AUTORCC，
# 但这个调用会确保 Qt 相关的所有默认配置到位
qt_standard_project_setup()

# qt_add_executable 是 Qt 提供的替代 CMake 原生 add_executable 的函数
# 它会额外处理 Qt 特有的编译需求（如自动链接 platform 插件）
qt_add_executable(JunkFusion)

add_subdirectory(qml)
# CMake 参数	用途	示例
# QML_FILES	专门放 .qml 文件。CMake 会对其进行类型检查、缓存编译，并自动支持 import。	QML_FILES main.qml TrackStrip.qml
# RESOURCES	放非 QML 资源：图片（.png）、字体（.ttf）、音频素材等。它们不会被注册为 QML 类型，只能通过 file:/// 或 qrc:/ 路径引用。

# 指定 C++ 源文件
# 这里只有 mainUI.cpp 一个文件（Qt 程序入口 + JuceProcessManager 头文件）
target_sources(JunkFusion PRIVATE
    mainUI.cpp
)
add_subdirectory(bridge)

# ==============================================================================
# 链接 Qt 库
#
# PRIVATE 意味着这些库只在 JunkFusion 内部使用，
# 不暴露给依赖 JunkFusion 的其他目标
#
# Qt6::Core   → QObject、QString、信号槽、事件循环
# Qt6::Quick  → QML 渲染引擎、Scene Graph
# Qt6::Qml    → QML 语言解析、JS 引擎
#
# 注：不需要显式链接 Qt6::QmlIntegration，
#     因为 qt_add_qml_module 已自动处理导入路径和资源编译
# ==============================================================================
target_link_libraries(JunkFusion
    PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
)
