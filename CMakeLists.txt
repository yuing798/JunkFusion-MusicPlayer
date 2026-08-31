cmake_minimum_required(VERSION 3.22)

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
find_package(unofficial-hash-library CONFIG REQUIRED) #哈希算法
find_package(cppzmq CONFIG REQUIRED) #tcp本地回环进程间通信
find_package(cpr CONFIG REQUIRED) #网络通信服务
find_package(Chromaprint CONFIG REQUIRED) #音频指纹提取
# target_link_libraries(main PRIVATE cpr::cpr)


if (MSVC)
    # 强制编译器以 UTF-8 编码读取源文件，并以 UTF-8 导出执行文件
    add_compile_options(/utf-8)
endif()

add_subdirectory(DLL)
add_subdirectory(AudioProcess)
add_subdirectory(Utils)


# 创建一个自定义目标（不生成文件，只为了触发动作）
#add_custom_target 定义了一个不生成具体文件的构建目标
#DEPENDS:它强制 CMake 在构建 DeployRuntime 之前，必须先构建完成这两个项目。
add_custom_target(DeployRuntime ALL
    DEPENDS JunkFusionDLL JunkFusionAudioProcess  # 依赖两个核心目标
    COMMENT "准备部署运行时库到 Flutter 目录..."
)

# $<CONFIG> 是 CMake 中极其核心的一个生成器表达式（Generator Expression），它的含义非常直白：
# 它会在构建（编译）时，被替换为当前激活的构建配置名称
# 他对应的是当前的CMAKE_BUILD_TYPE
target_compile_definitions(JunkFusionDLL PRIVATE
    $<$<CONFIG:Debug>:JF_DEBUG>
)
target_compile_definitions(JunkFusionAudioProcess PRIVATE
    $<$<CONFIG:Debug>:JF_DEBUG>
)

set_target_properties(JunkFusionDLL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/$<CONFIG>_bin"
)
set_target_properties(JunkFusionAudioProcess PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/$<CONFIG>_bin"
)

#将复制动作绑定到这个自定义目标上
add_custom_command(TARGET DeployRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        # ★ 修复隐患一：从构建目录（CMAKE_BINARY_DIR）下找 bin
        -D SRC_DIR="${CMAKE_SOURCE_DIR}/$<CONFIG>_bin"     
        # 用 $<CONFIG> 自动识别当前编译配置（Debug/Release）
        -D DST_DIR="${CMAKE_SOURCE_DIR}/junk_fusion_ui/build/windows/x64/runner/$<CONFIG>"
        -P "${CMAKE_SOURCE_DIR}/cmake/DeployBinaries.cmake"
    COMMENT "$<CONFIG>模式:正在同步所有动态库和后端进程到 Flutter 运行目录（已排除 .ilk/.pdb）..."
)