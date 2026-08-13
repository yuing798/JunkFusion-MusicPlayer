#pragma once

#include <memory>
#include <spdlog/logger.h>

namespace AudioUtils {
    void initAudioLogger(std::string logFilePath);
}