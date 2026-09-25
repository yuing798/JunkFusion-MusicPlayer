
add_library(JunkFusionCoordinator STATIC)

target_link_directories(JunkFusionCoordinator
    PRIVATE
        ${FFMPEG_LIBRARY_DIRS}
)

target_link_libraries(JunkFusionCoordinator
    PRIVATE
        SQLiteCpp#数据库系统
        spdlog::spdlog#日志系统
        
        juce::juce_audio_basics
        unofficial::hash-library#哈希算法库
        ${FFMPEG_LIBRARIES}
        juce::juce_gui_basics#基础UI组件
        cppzmq 
        cppzmq-static
        juce::juce_osc
        cpr::cpr #网络请求库
        Chromaprint::chromaprint
        TagLib::tag TagLib::tag_c TagLib::TagLib
    PUBLIC
        juce::juce_core
        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)

target_include_directories(JunkFusionCoordinator
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${FFMPEG_INCLUDE_DIRS}
)

target_sources(JunkFusionCoordinator
    PRIVATE
        libExport.cpp
        dllManager.cpp
)

# add_subdirectory(DartApi)
add_subdirectory(fileManage)
add_subdirectory(processSchedule)