#pragma once

#include <memory>
#include <spdlog/logger.h>

namespace AudioUtils {
    void initAudioLogger(std::string logFilePath, std::shared_ptr<spdlog::logger>& audioLogger);
}