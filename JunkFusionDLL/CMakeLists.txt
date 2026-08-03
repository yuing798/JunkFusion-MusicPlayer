
target_link_directories(JunkFusionDLL
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)

target_link_libraries(JunkFusionDLL
    PRIVATE
        SQLiteCpp#数据库系统
        spdlog::spdlog#日志系统
        juce::juce_core
        unofficial::hash-library#哈希算法库
        ${FFMPEG_LIBRARIES}
        juce::juce_gui_basics#基础UI组件
    PUBLIC
        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)

target_include_directories(JunkFusionDLL
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FFMPEG_INCLUDE_DIRS}
)

target_sources(JunkFusionDLL
    PRIVATE
        dllUtils.cpp
        libExport.cpp
)

add_subdirectory(DartApi)
add_subdirectory(fileManage)