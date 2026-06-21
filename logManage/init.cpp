#include "init.hpp"
#include "juce_core/juce_core.h"
#include <iostream>

void initLog(){
    try {
        juce::File exeFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/myapp.log", true);

        std::vector<spdlog::sink_ptr> sinks {file_sink};
        auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

        // 设置为全局默认日志器
        spdlog::set_default_logger(logger);

        // 设置日志级别和格式
        spdlog::set_level(spdlog::level::info);
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        spdlog::info("=== 应用启动 ===");
    } catch (const spdlog::spdlog_ex& ex) {
        // 初始化失败时的处理
        std::cerr << "日志系统初始化失败: " << ex.what() << std::endl;
    }
}