#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/playlistInfo.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
class PlayListManage {
private:
    CenterDbManage& dbManage;

public:
    PlayListManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    juce::Array<juce::var> getAllPlayListView();
    PlaylistInfo getCompleteInfo(int64_t id);
    bool createNewPlaylist(PlaylistInfo info);
};