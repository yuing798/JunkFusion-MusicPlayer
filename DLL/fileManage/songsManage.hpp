#pragma once

#include "TimeDomainSpecInsert.hpp"
#include "dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class SongsManage {
private:
    SQLite::Database& db;
    TimeDomainSpecInsert mTimeDomainSpecInsert;

    const int scrollPageRows = 200; // 滚动分页方式每一页的页数

public:
    explicit SongsManage(SQLite::Database& d);
    // ~SongsManage();

    InsertSongInfo insertSong(const juce::File& path);

    /** 统一分页入口，根据 SortMode 选择排序方式 */
    std::vector<SongInfo> getAllSongs();

    bool reverseMyLike(int64_t id);
    std::string getPathBySongId(int64_t songId); // 根据ID获得路径和歌曲时长
    bool deleteSongId(int64_t songId);           // 删除songId对应的条目
    void saveComment(juce::String text, int64_t songId);
    std::function<void(const char*)> onTimeDomainSpecInsertOver;
};
