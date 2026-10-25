#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/playlistInfo.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <vector>
class PlayListManage {
private:
    CenterDbManage& dbManage;

public:
    PlayListManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    nlohmann::json getAllPlayListView();
    PlaylistInfo getDetailInfo(int64_t id);
    PlaylistInfo getBasicInfo(int64_t id);
    int64_t createNewPlaylist(std::string title, std::optional<juce::File> coverPath);
    void addNewSongs(int64_t id, std::vector<int64_t> songIds);
    void removeSongs(int64_t id, std::vector<int64_t> songIds);
    void deletePlaylist(int64_t id);
    void renamePlayList(int64_t id, std::string title);
    void updateCover(int64_t id, std::string hash);
    void updateCover(int64_t id, int64_t songId);

    // 将一张封面图片拷贝到应用文件夹并转换为hash命名
    std::string coverPathCopyAndConvertToHash(juce::File coverPath);
};