cmake_minimum_required(VERSION 3.22)

project(JunkFusion VERSION 0.0.2)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.12  
)
FetchContent_MakeAvailable(JUCE)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

find_package(FFMPEG REQUIRED)
find_package(SQLiteCpp REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-hash-library CONFIG REQUIRED) #哈希算法
find_package(cppzmq CONFIG REQUIRED) #tcp本地回环进程间通信
find_package(cpr CONFIG REQUIRED) #网络通信服务
find_package(Chromaprint CONFIG REQUIRED) #音频指纹提取
find_package(taglib CONFIG REQUIRED) #歌曲艺术家元数据获取
find_path(SIGNALSMITH_STRETCH_INCLUDE_DIRS "signalsmith-stretch/signalsmith-stretch.h")
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)


if (MSVC)
    # 强制编译器以 UTF-8 编码读取源文件，并以 UTF-8 导出执行文件
    add_compile_options(/utf-8)
endif()

add_subdirectory(Coordinator)
add_subdirectory(AudioProcessor)
add_subdirectory(Utils)
add_subdirectory(Model)

target_compile_definitions(JunkFusionCoordinator PRIVATE
    $<$<CONFIG:Debug>:JF_DEBUG>
)
target_compile_definitions(JunkFusionAudioProcess PRIVATE
    $<$<CONFIG:Debug>:JF_DEBUG>
)
target_compile_definitions(Utils PRIVATE
    $<$<CONFIG:Debug>:JF_DEBUG>
)

set_target_properties(JunkFusionCoordinator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>_bin"
)
set_target_properties(JunkFusionAudioProcess PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/$<CONFIG>_bin"
)

target_link_libraries(Utils
    PUBLIC

        juce::juce_core
        SQLiteCpp
        
        
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog#日志系统
        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)

target_link_libraries(Models
    PUBLIC
        Utils
        juce::juce_core
        SQLiteCpp
        # ${FFMPEG_LIBRARIES}

    PRIVATE
        nlohmann_json::nlohmann_json
        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)
target_link_libraries(JunkFusionCoordinator 
    PRIVATE
        Models
        Utils
        fmt::fmt
)
target_link_libraries(JunkFusionAudioProcess
    PRIVATE
        Models
        Utils
)
add_custom_target(DeployRuntime ALL
    DEPENDS JunkFusionCoordinator JunkFusionAudioProcess  # 依赖两个核心目标
    COMMENT "准备部署运行时库到 Flutter 目录..."
)
add_custom_command(TARGET DeployRuntime POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -D SRC_DIR="${CMAKE_SOURCE_DIR}/build/$<CONFIG>_bin"     
        -D DST_DIR="${CMAKE_SOURCE_DIR}/junk_fusion_ui/build/windows/x64/runner/$<CONFIG>"
        -P "${CMAKE_SOURCE_DIR}/cmake/DeployBinaries.cmake"
    COMMENT "$<CONFIG>模式:正在同步所有动态库和后端进程到 Flutter 运行目录（已排除 .ilk）..."
)