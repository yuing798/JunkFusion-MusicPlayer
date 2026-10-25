add_library(Utils STATIC)

target_sources(Utils 
    PRIVATE #外界只需要看到头文件，不需要看到cpp的实现，所以这里用PRIVATE
        convertUtils.cpp #用来变量之间转换类型
        mathUtils.cpp #数学函数工具，通常只用在音频相关代码中
        Yvar.cpp #用来安全读取json的juce::var组合类
        otherUtils.cpp #其他乱七八糟的工具
        dbUtils.cpp #数据库工具
        stringUtils.cpp #字符串工具
)

target_include_directories(Utils
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)