#include "./fileUtils.hpp"

namespace FileUtils {
    std::string getFileLastModifiedTime(const juce::File& file) {
        return file.getLastModificationTime().toISO8601(false).toStdString();
    }

    std::string withTrailingSeparator(const std::string& dir) {
        if (dir.empty()) return dir;

        const char last = dir.back();
        if (last == '\\' || last == '/') return dir;

        const char sep = static_cast<char>(juce::File::getSeparatorChar());
        return dir + sep;
    }

    bool isPathUnderDir(const std::string& path, const std::string& dir) {
        if (path == dir) return true;

        const std::string prefix = withTrailingSeparator(dir);
        return path.rfind(prefix, 0) == 0;
    }

} // namespace FileUtils