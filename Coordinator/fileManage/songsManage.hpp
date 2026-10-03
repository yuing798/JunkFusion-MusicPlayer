#pragma once

#include "./WaveFormAnaly.hpp"
#include "Model/PlayInfo.hpp"
#include "Model/SongInfo.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <tpropertymap.h>
#include <vector>

class SongsManage {
private:
    SQLite::Database& db;
    WaveFormAnaly mWaveFormAnaly;

    // 获取元数据数组(比如多个艺术家)
    juce::StringArray getTags(TagLib::PropertyMap& map, const char* key);
    // 将字符串转换为数组并只提取从开头数的有效数据
    std::optional<int> getTagInt(TagLib::PropertyMap& map, const char* key);
    SongInfo searchSongInfo(SQLite::Statement& sql);

public:
    explicit SongsManage(SQLite::Database& d);
    // ~SongsManage();
    struct InsertState {
        juce::File path;
        std::string msg; // 如果成功没有信息
    };

    std::vector<InsertState> insertSongs(std::vector<juce::File>& paths);

    SongInfo getSongInfo(juce::String path);
    SongInfo getSongInfo(int64_t songId);
    juce::Array<juce::var> getAllSongs();

    bool reverseMyLike(int64_t id);
    PlayInfo getPlayInfo(int64_t songId); // 根据ID获得路径和歌曲时长
    void saveComment(juce::String text, int64_t songId);
    std::function<void(int64_t songId)> OnUpdateSongInfo;
    juce::Array<double> getTimeDomainSpec(int64_t songId); // 根据ID号获取时域图

    bool updatePlayCount(std::string path);
};
