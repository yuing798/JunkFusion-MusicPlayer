#pragma once

#include "juce_core/juce_core.h"
#include <string>
namespace FileUtils {
    std::string getFileLastModifiedTime(const juce::File& file);
}