#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/playlistInfo.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <vector>
class PlayListManage {
private:
    CenterDbManage& dbManage;

public:
    PlayListManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    juce::Array<juce::var> getAllPlayListView();
    PlaylistInfo getDetailInfo(int64_t id);
    bool createNewPlaylist(PlaylistInfo info);
    void addNewSongs(int64_t id, std::vector<int64_t> songIds);
    void removeSongs(int64_t id, std::vector<int64_t> songIds);
};