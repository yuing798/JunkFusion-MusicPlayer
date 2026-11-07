#pragma once

#include "juce_core/juce_core.h"
#include <string>
namespace FileUtils {
    std::string getFileLastModifiedTime(const juce::File& file);

    // 路径结尾补一个分隔符号
    std::string withTrailingSeparator(const std::string& dir);

    /// path 是不是 dir 本身或它下面的东西
    bool isPathUnderDir(const std::string& path, const std::string& dir);
} // namespace FileUtils