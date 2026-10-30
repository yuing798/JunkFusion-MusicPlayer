#pragma once

#include "Coordinator/CenterDbManage/dbContext.hpp"
#include "SongImport.hpp"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include <unordered_map>
#include <utility>
#include <vector>
// 文件夹检查,每一分钟检查一次，触发扫描后停止三分钟，窗口不可见的话直接关闭计时器
class FolderCheckTimer : public juce::Timer {
private:
    DbContext& mDbContext;
    SongImport& mSongImport;

    enum class SqlState {
        Update, // 这个文件夹时间戳变了
        Delete, // 这个文件夹被删除了
        Add,    // 这个文件夹是新添加的
    };
    struct DirState {
        SqlState sqlState{SqlState::Add};
        bool isRoot{false};
    };

public:
    FolderCheckTimer(DbContext& dbContext, SongImport& songImport);
    // 比较数据库中的文件夹和实际文件夹，
    std::unordered_map<juce::File, DirState> compareAllDirState();

    void timerCallback() override;

    // 导入只能导根目录，子目录通过根目录递归获取
    void importNewFolder(const juce::File& folderPath);
};