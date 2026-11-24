# =============================================================================
# PluginHost —— 所有插件（VST3）相关逻辑
#
# ── 为什么这个文件夹里**没有静态库目标** ──────────────────────────────────
# JUCE 的模块目标（juce::juce_audio_processors_headless 等）是 INTERFACE 库，
# 模块的 .cpp 通过 INTERFACE_SOURCES 加进「链接它的那个 target」。如果我们在这里
# 再建一个 STATIC 库去链接 JUCE，整个 VST3 SDK 会被编译两遍（静态库里一份、
# exe 里一份）：轻则编译时间翻倍，重则链接期符号重复、静态库那半永远不被拉进来。
# 所以这里只放源码清单，**谁需要哪几个 .cpp 就自己 target_sources 拿走**，
# 各自编译（这点重复量可以忽略）。
#
# ── 文件分工 ──────────────────────────────────────────────────────────────
#   PluginTypes.hpp           纯数据 + 枚举（无 JUCE，可裸 cl 编译测试）
#   PluginCategory.hpp/.cpp   §2 分类规则（纯 std，同上）
#   PluginScanProtocol.*      扫描子进程的行协议（纯 std，同上）
#   PluginPathScanner.*       递归找 .vst3（juce_core）
#   PluginEnumerate.*      用 JUCE headless 枚举元数据（juce_audio_processors_headless）
#   ScanMain.cpp              ★扫描子进程入口★ → JunkFusionPluginScan.exe
#   PluginCatalogManage.*     plugins / pluginDirs 表的读写（Coordinator）
#   PluginScannerProcess.*    拉起/监管扫描子进程，断点续扫（Coordinator）
#   PluginScanJob.*           后台扫描线程 + 进度状态（Coordinator）
# =============================================================================

# 纯逻辑：扫描子进程和 Coordinator 都要（一个负责编码，一个负责解码）
set(PLUGIN_HOST_PURE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginCategory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginScanProtocol.cpp
)

# 需要 JUCE 的扫描侧：找文件 + 枚举
set(PLUGIN_HOST_JUCE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginPathScanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginEnumerate.cpp
)

# Coordinator 侧：入库 + 监管子进程 + 后台任务
set(PLUGIN_HOST_COORDINATOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginCatalogManage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginScannerProcess.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PluginScanJob.cpp
)

# =============================================================================
# 扫描子进程（独立 target / 独立 exe）
#
# 这是规格 §3.3 的硬要求：枚举本身就会 LoadLibrary 那个插件 DLL，坏插件在枚举
# 阶段就能把进程带走。它跑在独立进程里，死了主进程重开一个继续扫，音频进程
# 完全不受影响。
#
# 刻意只链接 headless 版：子进程不需要任何图形/窗口代码，崩溃面越小越好。
# =============================================================================
juce_add_console_app(JunkFusionPluginScan
    COMPANY_NAME "YusekX"
)

target_sources(JunkFusionPluginScan
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ScanMain.cpp
        ${PLUGIN_HOST_PURE_SOURCES}
        ${PLUGIN_HOST_JUCE_SOURCES}
)

target_include_directories(JunkFusionPluginScan
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(JunkFusionPluginScan
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        # 只开 VST3 宿主。VST2 需要额外塞 VST2 SDK 头文件，这里明确关掉；
        # AU / LADSPA / LV2 同理（规格 §1.1：只扫 .vst3）。
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
)

target_link_libraries(JunkFusionPluginScan
    PRIVATE
        juce::juce_audio_processors_headless
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MSVC)
  target_compile_options(JunkFusionPluginScan
        PRIVATE
            # 和主工程其它 target 保持一致：源码里有中文，不按 UTF-8 读会报
            # "newline in string literal"
            /utf-8
            /wd4244
            /wd4267
    )

  # 入口改成 wmain：中文用户名下的缓存路径是 `C:\Users\张三\...`，
  # 而 main(argc, argv) 拿到的是系统 ANSI 代码页(GBK)的字节，跨不过
  # "非中文系统 + 中文路径"这个组合。wmain 能拿到 UTF-16 的原始命令行。
  # （ScanMain.cpp 里同时保留了 #if JUCE_WINDOWS / wmain 和 POSIX main 两个入口）
  set_target_properties(JunkFusionPluginScan PROPERTIES
        LINK_FLAGS "/ENTRY:wmainCRTStartup"
    )
endif()

# =============================================================================
# 音频进程侧的插件宿主（全局效果器链）
#
# 这一步会让 JunkFusionAudioProcess **第一次**把整个 VST3 SDK 编进去
# （juce_audio_processors 的模块源码带 JUCE_PLUGINHOST_VST3=1 重编），
# 大约多花几分钟，属于一次性成本。
#
# 为什么音频进程用的是「带 UI」的格式管理器而不是 headless：
# 按实验台结论，插件编辑器最终要在音频进程里开（那边有消息循环），
# headless 装出来的实例 hasEditor() 恒为 false，迟早要换。
# =============================================================================
target_sources(JunkFusionAudioProcess
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginInstanceFactory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginChain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginEditorWindow.cpp
)

target_compile_definitions(JunkFusionAudioProcess
    PRIVATE
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
)

# =============================================================================
# Coordinator 侧：直接加进 DLL 的源码清单（不新建 target）
# =============================================================================
target_sources(JunkFusionCoordinator
    PRIVATE
        ${PLUGIN_HOST_PURE_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/PluginPathScanner.cpp
        ${PLUGIN_HOST_COORDINATOR_SOURCES}
)

# 扫描子进程和两个主程序放同一个输出目录，部署脚本一次性同步
set_target_properties(JunkFusionPluginScan PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>_bin"
)
