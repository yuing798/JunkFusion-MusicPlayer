cmake_minimum_required(VERSION 3.22)

# 定义你的项目名称和版本
project(JunkFusion VERSION 0.0.1)

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

# ==============================================================================
# 2. 定义 GUI App 目标
# ==============================================================================
juce_add_gui_app(JunkFusion
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "YusekX"
    PRODUCT_NAME "JunkFusion"
)

# ==============================================================================
# 3. 添加你的 C++ 源码文件
# ==============================================================================
target_sources(JunkFusion
    PRIVATE
        Main.cpp
        MainComponent.h
        MainComponent.cpp
        # 未来你可以把这里分组，比如：
        # AudioEngine.cpp
        # AiProcessManager.cpp
        # DatabaseManager.cpp
)

# ==============================================================================
# 4. 预编译宏定义 (核心配置区)
# ==============================================================================
target_compile_definitions(JunkFusion
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        
        # --- VST 宿主核心开关 (必须打开，否则无法加载第三方 VST 插件) ---
        JUCE_PLUGINHOST_VST=0   # VST2 已经淘汰，直接关闭
        JUCE_PLUGINHOST_VST3=1  # 开启 VST3 插件加载支持
        JUCE_PLUGINHOST_AU=1    # 开启苹果 AU 插件加载支持 (在 Mac 上编译时生效)
        
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:JunkFusion,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:JunkFusion,JUCE_VERSION>"
)

# ==============================================================================
# 5. 链接库依赖 (JUCE 模块与第三方库)
# ==============================================================================
target_link_libraries(JunkFusion
    PRIVATE
        # --- JUCE 官方音频与 UI 模块 ---
        juce::juce_audio_utils       # 包含底层声卡设备管理
        juce::juce_audio_processors  # 包含 AudioProcessorGraph 和 VST 宿主类!
        juce::juce_dsp               # 包含 EQ、滤波等高阶数字信号处理算法
        juce::juce_gui_extra         # 包含高级 UI 组件
        
        # --- 未来第三方库的占位符
        # FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil FFmpeg::swresample
        # SQLite::SQLite3
        # ONNX::onnxruntime

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)