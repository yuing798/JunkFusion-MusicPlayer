#pragma once

#include "./WaveFormAnaly.hpp"
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
    WaveFormAnaly mWaveFormAnaly;

    const int scrollPageRows = 200; // 滚动分页方式每一页的页数

public:
    explicit SongsManage(SQLite::Database& d);
    // ~SongsManage();

    InsertSongInfo insertSong(const juce::File& path);

    /** 统一分页入口，根据 SortMode 选择排序方式 */
    std::vector<SongInfo> getAllSongs();

    bool reverseMyLike(int64_t id);
    std::string getPathBySongId(int64_t songId); // 根据ID获得路径和歌曲时长
    void saveComment(juce::String text, int64_t songId);
    std::function<void(const char*)> onTimeDomainSpecInsertOver;   // 时域图插入数据库完成
    juce::Array<double> getTimeDomainSpecBySongId(int64_t songId); // 根据ID号获取时域图
};
