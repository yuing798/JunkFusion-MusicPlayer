#pragma once

#include "dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include "libExport.h"
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

    const int scrollPageRows = 200; // 滚动分页方式每一页的页数

public:
    explicit SongsManage(SQLite::Database& d);
    ~SongsManage();

    std::optional<SongInfo> insertSong(const juce::File& path);

    /** 获取歌曲总数 */
    std::optional<int> getTotalSongCount();

    /** 统一分页入口，根据 SortMode 选择排序方式 */
    std::vector<SongInfo> getAllSongs();

    bool reverseMyLike(int64_t id);
    std::string getImageHashBySongId(int64_t id);
    std::string getPathBySongId(int64_t songId); // 根据ID获得路径
    bool deleteSongId(int64_t songId);           // 删除songId对应的条目
    void saveComment(juce::String text, int64_t songId);
};

extern "C" {
    LIB_EXPORT void dbInit();
    LIB_EXPORT int getAllSongCount();
    LIB_EXPORT bool toggleMyLike(int64_t songId);
    LIB_EXPORT const char* getAllSongs();
    LIB_EXPORT void saveComment(int64_t songId, const char* commentText);
    LIB_EXPORT void freeString(const char* str);
}