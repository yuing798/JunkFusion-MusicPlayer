#pragma once

#include "../CenterDbManage/CenterDbManage.hpp"
#include "Model/PlayInfo.hpp"
#include "Model/SongInfo.hpp"
#include "Utils/Yvar.hpp"
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

    SongInfo getBasicInfoCol(SQLite::Statement& query);
    SongInfo getDetailInfoCol(SQLite::Statement& query);

public:
    explicit SongsManage(CenterDbManage& dbManage) : dbManage(dbManage) {};

    // 得到某一首歌的详细SongInfo信息
    SongInfo getDetailSongInfo(int64_t songId);
    SongInfo getBasicSongInfo(int64_t songId);

    // 得到所有歌曲的基本信息用于UI视图
    Yvar getAllSongsBasicInfo();

    void reverseMyLike(int64_t id);
    PlayInfo getPlayInfo(int64_t songId); // 根据ID获得路径和歌曲时长

    void updatePlayCount(std::string path);
    /**
     * @brief  检查一首歌是否在歌曲表里面,返回代表的songId
     *
     * @param path
     * @return int64_t songId
     */

    Yvar getSingleImportSongs();

    // 获取元数据数组(比如多个艺术家)
    std::vector<std::string> getTags(TagLib::PropertyMap& map, const char* key);
    // 将字符串转换为数组并只提取从开头数的有效数据
    std::optional<int> getTagInt(TagLib::PropertyMap& map, const char* key);
};
