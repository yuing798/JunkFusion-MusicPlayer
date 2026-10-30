#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class SongFolderManage {
private:
    CenterDbManage& dbManage;
    std::unordered_map<std::string, int64_t> mPathToIdMap;

public:
    SongFolderManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    Yvar getAllRootDirView();
    Yvar getChildDirAndSongIdList(const juce::File& parentPath);
    std::vector<juce::File> getAllDir();
    std::vector<juce::File> getAllRootDirOrChildDir(bool isRoot);

    // 检查这个文件夹是否已经导入，防止全量扫描
    void insertFolderToTable(const juce::File& dir, bool isRoot);
    bool isDirHasImport(const juce::File& dir);
    // 这个文件夹是否为在应用中注册的根目录
    bool isDirRoot(const juce::File& dir);

    // 通过比较文件夹的lastModifiedTime来判断文件夹是否更新
    bool checkSingleDirUpdate(const juce::File& dir);

    /**
     * @brief 递归检查当前文件夹和子文件夹是否更新，缩小到最小更新范围
     *
     * @param dir
     * @return std::vector<juce::File>
     */
    std::vector<juce::File> checkDirUpdateRecursion(const juce::File& dir);
};