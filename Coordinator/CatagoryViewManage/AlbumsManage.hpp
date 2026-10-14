#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/AlbumInfo.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <cstdint>
#include <optional>
#include <vector>
class AlbumsManage {
private:
    CenterDbManage& dbManage;

public:
    AlbumsManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    juce::Array<juce::var> getAllAlbumsView();

    std::optional<int64_t> checkAlbumExist(
        juce::String albumTitle,
        juce::StringArray artists,
        std::optional<juce::String> hash
    );

    std::optional<int64_t> insertAlbum(
        juce::String albumTitle,
        juce::StringArray artists,
        std::optional<juce::String> hash
    );
    std::optional<int64_t> getAlbumId(int64_t songId);
    std::vector<int64_t> getSongsList(int64_t albumId);
    juce::var getCompleteInfo(int64_t albumId);
    AlbumInfo getBasicInfoCol(SQLite::Statement& sql);
}; // namespace AlbumsManage