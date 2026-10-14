#pragma once

#include "../CenterDbManage/CenterDbManage.hpp"
#include "../SongsInsertHelper/WaveFormAnaly.hpp"
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
#include <utility>
#include <variant>
#include <vector>

class SongsManage {
private:
    CenterDbManage& dbManage;

    // 获取元数据数组(比如多个艺术家)
    juce::StringArray getTags(TagLib::PropertyMap& map, const char* key);
    // 将字符串转换为数组并只提取从开头数的有效数据
    std::optional<int> getTagInt(TagLib::PropertyMap& map, const char* key);

    SongInfo getBasicInfoCol(SQLite::Statement& query);
    SongInfo getCompleteInfoCol(SQLite::Statement& query);

public:
    explicit SongsManage(CenterDbManage& dbManage) : dbManage(dbManage) {};

    // SongsManage::insertSong的返回结果
    struct InsertSongResult {

        SongInfo info;
        std::optional<juce::StringArray> albumArtists;
        bool titleIsFileName{false}; // title是否为文件名
    };

    /**
     * @brief
     *
     * @param path
     * @return std::pair<std::string, InsertSongResult> first:如果错误返回的信息,second:正确结果
     */
    std::variant<std::string, InsertSongResult> insertSong(const juce::File& path);

    // 得到某一首歌的全部SongInfo信息
    SongInfo getCompleteSongInfo(int64_t songId);

    // 得到所有歌曲的基本信息用于UI视图
    juce::Array<juce::var> getAllSongsBasicInfo();

    void reverseMyLike(int64_t id);
    PlayInfo getPlayInfo(int64_t songId); // 根据ID获得路径和歌曲时长

    void updatePlayCount(std::string path);
};
