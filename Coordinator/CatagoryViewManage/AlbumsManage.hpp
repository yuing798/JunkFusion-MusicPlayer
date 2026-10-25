#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/AlbumInfo.hpp"
#include "Utils/Yvar.hpp"
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

    Yvar getAllAlbumsView();

    std::optional<int64_t> checkAlbumExist(
        std::string albumTitle,
        std::vector<std::string> artists,
        std::optional<std::string> hash
    );

    std::optional<int64_t> insertAlbum(
        std::string albumTitle,
        std::vector<std::string> artists,
        std::optional<std::string> hash
    );
    std::optional<int64_t> getAlbumId(int64_t songId);
    std::vector<int64_t> getSongsList(int64_t albumId);
    Yvar getDetailInfo(int64_t albumId);
    AlbumInfo getBasicInfoCol(SQLite::Statement& sql);
}; // namespace AlbumsManage