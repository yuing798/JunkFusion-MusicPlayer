add_library(Models STATIC)

target_sources(Models
    PRIVATE 
        SongInfo.cpp
        PlayInfo.cpp
)

target_link_libraries(Models
    PUBLIC

        juce::juce_core

    PRIVATE
        
        spdlog::spdlog#日志系统

        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)
target_include_directories(Models
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)