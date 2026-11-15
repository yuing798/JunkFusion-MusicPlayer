#include "./PluginPathScanner.hpp"

#include <algorithm>
#include <set>

namespace PluginHost {

    bool PathScanner::hasVst3Extension(const juce::File& file) {
        // hasFileExtension 已经是大小写不敏感的，且对"目录"同样按名字判断，
        // 所以 bundle 目录和裸 DLL 用同一个判断就能都覆盖。
        return file.hasFileExtension(".vst3");
    }

    juce::String PathScanner::dedupeKey(const juce::File& file) {
        return file.getFullPathName().toLowerCase();
    }

    void PathScanner::scanDirectory(
        const juce::File& dir,
        int depth,
        int maxDepth,
        Result& out
    ) const {
        if (depth > maxDepth) return;

        ++out.directoriesVisited;

        juce::Array<juce::File> children;
        dir.findChildFiles(children, juce::File::findFilesAndDirectories, false);

        // 排序只为一件事：让 index 在两次扫描之间稳定，断点续扫才有意义
        std::sort(
            children.begin(),
            children.end(),
            [](const juce::File& a, const juce::File& b) {
                return a.getFileName() < b.getFileName();
            }
        );

        for (const auto& child : children) {
            ++out.filesVisited;

            // 命中 .vst3 就是模块，**不再往里钻**：
            // 钻进去会把 bundle 里的 Contents/x86_64-win/Foo.vst3 也收进来，
            // 同一个插件就变成两行（见头文件里 bundle 那一段）。
            if (hasVst3Extension(child)) {
                out.modules.push_back(child);
                continue;
            }

            if (!child.isDirectory()) continue;

            // 跳过符号链接目录：Windows 的 junction 能造出环，递归没有尽头
            if (child.isSymbolicLink()) continue;

            scanDirectory(child, depth + 1, maxDepth, out);
        }
    }

    PathScanner::Result PathScanner::scan(const juce::File& root, int maxDepth) const {
        Result result;
        if (root == juce::File{} || !root.isDirectory()) return result;

        scanDirectory(root, 0, maxDepth, result);
        return result;
    }

    PathScanner::Result PathScanner::scanAll(
        const std::vector<juce::File>& roots,
        int maxDepth
    ) const {
        Result merged;
        std::set<juce::String> seen;

        for (const auto& root : roots) {
            const Result one = scan(root, maxDepth);
            merged.filesVisited += one.filesVisited;
            merged.directoriesVisited += one.directoriesVisited;

            for (const auto& module : one.modules) {
                const auto key = dedupeKey(module);
                if (!seen.insert(key).second) continue;
                merged.modules.push_back(module);
            }
        }

        return merged;
    }

} // namespace PluginHost
