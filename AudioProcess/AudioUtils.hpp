#pragma once

#include "juce_core/juce_core.h"
#include <memory>
#include <spdlog/logger.h>

namespace AudioUtils {
    void initAudioLogger(juce::File cacheDir);
}