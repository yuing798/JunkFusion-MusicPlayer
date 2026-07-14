#pragma once

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include "BridgeNames.h"
#include "dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class SongsManage {
private:
    SQLite::Database& db;

    /** 使用 ICU Collator 按 title 排序后为所有歌曲重新分配 nameId */
    void rebuildNameIds();

    const int traditionalPageRows = std::stoi(B_other::traditionalPageRows);//传统分页方式每一页的页数
    const int scrollPageRows = std::stoi(B_other::scrollPageRows);//滚动分页方式每一页的页数
    
public:
    explicit SongsManage(SQLite::Database& d);
    ~SongsManage();

    /**
     * 插入一首歌，带事务保护。
     *
     * 防重复策略：
     *   1. 数据库层：filePath 有 UNIQUE 约束
     *   2. 代码层：插入前先查询 filePath，比较 fileSize 和 lastModifiedTime
     *      - 完全相同 → 视为重复，跳过插入
     *      - 不同 → 文件已更新，更新旧记录
     *      - 不存在 → 正常插入
     *
     * @param info     歌曲信息
     */
    bool insertSong(juce::File& path);

    /** 获取歌曲总数 */
    std::optional<int> getTotalSongCount();

    enum class SortMode {
        ByAddTime,
        ByName,
        ByPlayTimes
    };
    /** 统一分页入口，根据 SortMode 选择排序方式 */
    std::vector<SongInfo> getSongPage(int page, bool ascending, SortMode mode);

    bool reverseMyLike(int64_t id);

};

class songsManageBuilder : public juce::OptionsBuilder<juce::WebBrowserComponent::Options>{
public:

    juce::WebBrowserComponent::Options buildOptions(const juce::WebBrowserComponent::Options& initial) override;
};