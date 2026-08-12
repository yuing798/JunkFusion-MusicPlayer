cmake_minimum_required(VERSION 3.22)

# 定义你的项目名称和版本
project(JunkFusion VERSION 0.0.2)

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
#find_package获得的目标内部已经使用了target_include_directory,所以不需要自己写

if (MSVC)
    # 强制编译器以 UTF-8 编码读取源文件，并以 UTF-8 导出执行文件
    add_compile_options(/utf-8)
endif()

# ==============================================================================
# 2. 定义 GUI App 目标
# ==============================================================================
add_library(JunkFusionDLL SHARED)#编译为dll

juce_add_gui_app(JunkFusionAudioProcess
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "YusekX"
    PRODUCT_NAME "JunkFusionAudioProcess"
    NEEDS_WEB_BROWSER FALSE
    NEEDS_WEBVIEW2 FALSE
)

target_sources(JunkFusionAudioProcess
    PRIVATE
        Main.cpp
)
add_subdirectory(Utils)
# add_subdirectory(image)
add_subdirectory(JunkFusionDLL)
# JUCE 内部会覆盖输出目录到 JunkFusion_artefacts，这里强制改回统一目录
set_target_properties(JunkFusionAudioProcess PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

    
# 创建一个自定义目标（不生成文件，只为了触发动作）
add_custom_target(DeployRuntime ALL
    DEPENDS JunkFusionDLL JunkFusionAudioProcess  # 依赖两个核心目标
    COMMENT "准备部署运行时库到 Flutter 目录..."
)
if(WIN32)
set(FLUTTER_EXE_DIR "${CMAKE_SOURCE_DIR}/junk_fusion_ui/build/windows/x64/runner/$<CONFIG>")
endif()

#将复制动作绑定到这个自定义目标上
add_custom_command(TARGET DeployRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        # ★ 修复隐患一：从构建目录（CMAKE_BINARY_DIR）下找 bin
        -D SRC_DIR="${CMAKE_SOURCE_DIR}/bin"     
        # 用 $<CONFIG> 自动识别当前编译配置（Debug/Release）
        -D DST_DIR="${FLUTTER_EXE_DIR}"
        -P "${CMAKE_SOURCE_DIR}/cmake/DeployBinaries.cmake"
    COMMENT "正在同步所有动态库和后端进程到 Flutter 运行目录（已排除 .ilk/.pdb）..."
)
# ==============================================================================
# 4. 预编译宏定义 (核心配置区)
# ==============================================================================
target_compile_definitions(JunkFusionAudioProcess
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
target_include_directories(JunkFusionAudioProcess
    PRIVATE
        ${FFMPEG_INCLUDE_DIRS}
)
target_link_directories(JunkFusionAudioProcess
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)
target_link_libraries(JunkFusionAudioProcess
    PRIVATE
        # --- 项目内部库 ---
        # JunkFusionBinaryData

        # --- JUCE 官方音频与 UI 模块 ---
        juce::juce_audio_utils       # 包含底层声卡设备管理
        juce::juce_audio_processors  # 包含 AudioProcessorGraph 和 VST 宿主类!
        # juce::juce_dsp               # 包含 EQ、滤波等高阶数字信号处理算法
        # juce::juce_gui_extra         # 包含高级 UI 组件
        juce::juce_gui_basics#基础UI组件
        juce::juce_core
        
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
# cmake函数中可以使用的宏：
# ${ARGN}	最常用。包含所有未被形参匹配的额外参数列表。
# $<ARGC>	传入实参的总个数（此例中为 2）。
# $<ARGV>	所有实参的完整列表（包括匹配的）。
# $<ARGV0>、$<ARGV1>...	按索引访问具体参数（0代表第一个）。
# $<ARGN0>、$<ARGN1>...	按索引访问剩余参数（需先开启 cmake_policy(SET CMP0053 NEW)）。