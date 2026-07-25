#pragma once

#include "../engineAudio/model.h"
#include "BridgeNames.h"
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

    const int scrollPageRows = std::stoi(B_other::scrollPageRows); // 滚动分页方式每一页的页数

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
    std::optional<playInfo> getPlayInfoBySongId(int64_t songId); // 根据ID获得播放信息
    bool deleteSongId(int64_t songId);                           // 删除songId对应的条目
    void saveComment(juce::String text, int64_t songId);
};

class songsManageBuilder : public juce::OptionsBuilder<juce::WebBrowserComponent::Options> {
public:
    juce::WebBrowserComponent::Options
    buildOptions(const juce::WebBrowserComponent::Options& initial) override;
};