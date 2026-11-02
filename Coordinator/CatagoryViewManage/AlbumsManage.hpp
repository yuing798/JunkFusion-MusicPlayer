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

    /**
     * @brief 专辑里一首歌都不剩了，就把 albums 这一行删掉
     *
     * 歌被删掉之后调（比如文件夹被删、目录里的音频文件被删），由调用方负责
     * 通知前端。不会误删：还有歌引用这个 albumId 时直接返回 nullopt。
     *
     * @param albumId 要检查的专辑
     * @return 真的删掉了返回 albumId；还有歌/专辑不存在/出错都返回 nullopt
     */
    std::optional<int64_t> removeAlbumIfEmpty(int64_t albumId);
}; // namespace AlbumsManage