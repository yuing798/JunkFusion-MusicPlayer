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

    /**
     * @brief 联网搜索某张专辑的封面，下载并存进 albums.hash
     *
     * 导入阶段已经不再联网了，封面统一走这里按需补（前端右键「联网搜索专辑封面」）。
     * 搜索词是「主艺术家 + 专辑名」，群星这类名字会被跳过；搜不到再用纯专辑名兜底。
     *
     * @param albumId 专辑 id
     * @param imageDir 封面缓存目录（<缓存目录>/image）
     * @return 新的封面 hash；搜不到或下载失败返回 nullopt
     */
    std::optional<std::string> searchAndSaveCover(int64_t albumId, const juce::File& imageDir);

private:
    /** @brief 向 iTunes 搜一条结果，返回 artworkUrl100；失败 nullopt */
    static std::optional<std::string>
        searchCoverURL(const std::string& term, const std::string& entity);
}; // namespace AlbumsManage