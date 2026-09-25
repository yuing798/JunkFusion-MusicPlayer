add_library(JunkFusionUtils STATIC)

target_sources(JunkFusionUtils 
    PRIVATE #外界只需要看到头文件，不需要看到cpp的实现，所以这里用PRIVATE
        convertUtils.cpp #用来变量之间转换类型
        mathUtils.cpp #数学函数工具，通常只用在音频相关代码中
        Yvar.cpp #用来安全读取json的juce::var组合类
        otherUtils.cpp #其他乱七八糟的工具
)

target_link_libraries(JunkFusionUtils
    PUBLIC

        juce::juce_core

    PRIVATE
        
        spdlog::spdlog#日志系统

        juce::juce_recommended_config_flags#基础编译与调试配置
        juce::juce_recommended_lto_flags#链接时代码优化
        juce::juce_recommended_warning_flags#推荐的警告级别
)
target_include_directories(JunkFusionUtils
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)