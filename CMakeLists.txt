cmake_minimum_required(VERSION 3.22)

# 定义你的项目名称和版本
project(JunkFusion VERSION 0.0.2)

# ==============================================================================
# 1. 引入 JUCE 框架
# ==============================================================================
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12  
)
FetchContent_MakeAvailable(JUCE)

find_package(FFMPEG REQUIRED)
find_package(SQLiteCpp REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-hash-library CONFIG REQUIRED)

if (MSVC)
    # 强制编译器以 UTF-8 编码读取源文件，并以 UTF-8 导出执行文件
    add_compile_options(/utf-8)
endif()

# ==============================================================================
# 2. 定义 GUI App 目标
# ==============================================================================
juce_add_gui_app(JunkFusionBackend
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "YusekX"
    PRODUCT_NAME "JunkFusionBackend"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_WEBVIEW2 FALSE
)

target_sources(JunkFusionBackend
    PRIVATE
        Main.cpp
)
add_subdirectory(Utils)
add_subdirectory(fileManage)
add_subdirectory(image)
# JUCE 内部会覆盖输出目录到 JunkFusion_artefacts，这里强制改回统一目录
set_target_properties(JunkFusionBackend PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ==============================================================================
# 4. 预编译宏定义 (核心配置区)
# ==============================================================================
target_compile_definitions(JunkFusionBackend
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        
        # --- VST 宿主核心开关 (必须打开，否则无法加载第三方 VST 插件) ---
        JUCE_PLUGINHOST_VST=0   # VST2 已经淘汰，直接关闭
        JUCE_PLUGINHOST_VST3=1  # 开启 VST3 插件加载支持
        JUCE_PLUGINHOST_AU=1    # 开启苹果 AU 插件加载支持 (在 Mac 上编译时生效)
)

# ==============================================================================
# 5. 链接库依赖 (JUCE 模块与第三方库)
# ==============================================================================
target_include_directories(JunkFusionBackend
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)
target_link_directories(JunkFusionBackend
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)
target_link_libraries(JunkFusionBackend
    PRIVATE
        # --- 项目内部库 ---
        JunkFusionBinaryData

        # --- JUCE 官方音频与 UI 模块 ---
        juce::juce_audio_utils       # 包含底层声卡设备管理
        juce::juce_audio_processors  # 包含 AudioProcessorGraph 和 VST 宿主类!
        juce::juce_dsp               # 包含 EQ、滤波等高阶数字信号处理算法
        # juce::juce_gui_extra         # 包含高级 UI 组件
        # juce::juce_gui_basics#基础UI组件
        
        # 第三方库
        ${FFMPEG_LIBRARIES}#解码层和重采样层
        SQLiteCpp#数据库系统
        spdlog::spdlog#日志系统
        unofficial::hash-library#哈希算法库
        # ONNX::onnxruntime
        # PkgConfig::ICU#字符串排序功能

    PUBLIC
        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)

juce_generate_juce_header(JunkFusionBackend)