# ==============================================================================
# QML 模块定义 — qt_add_qml_module
# 这是 Qt 6.2+ 推荐的 QML 管理方式，替代过去手写 .qrc 文件
#
# 参数说明：
#   URI        → QML 模块的唯一标识符（类比 Java 的包名 com.company.module）
#                在 QML 代码中通过 import JunkFusion 来使用这个模块
#   VERSION    → 模块版本号，QML import 时可以指定版本
#   QML_FILES  → 模块包含的所有 .qml 文件
#
# 这个命令会自动：
# - 生成 .qrc 资源文件（不需要手写 qml.qrc）
# - 注册 QML 类型（让 QML 引擎能找到这些文件）
# - 设置正确的 qrc 路径
#
# 加载方式（在 C++ 中）：
#   engine.loadFromModule("JunkFusion", "main");
#   → 加载模块 JunkFusion 中的 main.qml 文件
# ==============================================================================
set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(JunkFusion
    URI "JunkFusion"
    VERSION 1.0
    QML_FILES
        Theme.qml
        Main.qml
        LeftColumn.qml
        AllMusicPage.qml
        TitleBar.qml
        component/TitleBar.qml
)