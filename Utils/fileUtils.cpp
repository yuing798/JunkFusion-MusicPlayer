#include "./fileUtils.hpp"

namespace FileUtils {
    std::string getFileLastModifiedTime(const juce::File& file) {
        return file.getLastModificationTime().toISO8601(false).toStdString();
    }
} // namespace FileUtils