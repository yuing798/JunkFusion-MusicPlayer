juce_add_console_app(JunkFusionAudioProcess
    COMPANY_NAME "YusekX"
)    

target_sources(JunkFusionAudioProcess
    PRIVATE
        Main.cpp
        AudioRingBuffer.cpp
        DeviceManager.cpp 
        ffmpegDecoder.cpp
        GodProcessor.cpp
        AudioPreProcess.cpp
)

add_subdirectory(processSchedule)

target_compile_definitions(JunkFusionAudioProcess
    PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_console_app` call
        JUCE_USE_CURL=0)    # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_console_app` call

target_include_directories(JunkFusionAudioProcess
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}
)
target_link_directories(JunkFusionAudioProcess
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)

target_link_libraries(JunkFusionAudioProcess
    PRIVATE

        # --- JUCE 官方音频与 UI 模块 ---
        juce::juce_audio_utils       # 包含底层声卡设备管理
        juce::juce_audio_processors  # 包含 AudioProcessorGraph 和 VST 宿主类!
        # juce::juce_dsp               # 包含 EQ、滤波等高阶数字信号处理算法
        # juce::juce_gui_extra         # 包含高级 UI 组件
        juce::juce_core
        cppzmq 
        cppzmq-static
        juce::juce_osc
        
        # 第三方库
        ${FFMPEG_LIBRARIES}#解码层和重采样层
        spdlog::spdlog#日志系统
        # ONNX::onnxruntime
        # PkgConfig::ICU#字符串排序功能

    PUBLIC
        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)
