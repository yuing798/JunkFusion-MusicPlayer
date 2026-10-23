#include "./AlbumsManage.hpp"
#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/AlbumInfo.hpp"
#include "Model/SongInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include "Utils/stringUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <algorithm>
#include <cstdint>
#include <fmt/format.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

juce::Array<juce::var> AlbumsManage::getAllAlbumsView() {
    SQLite::Statement selectSql{
        dbManage.readDb(),
        "SELECT albumId,title,albumArtists,hash FROM albums"
    };
    juce::Array<juce::var> albums;
    while (selectSql.executeStep()) {
        AlbumInfo info{};
        info.albumId = selectSql.getColumn("albumId").getInt64();
        info.title = selectSql.getColumn("title").getString();
        info.albumArtists =
            Yvar{juce::JSON::fromString(selectSql.getColumn("albumArtists").getString())}
                .toStringArray();
        info.hash = selectSql.getColumn("hash").getString();
        juce::var finalObj{info.toBasicJson()};
        albums.add(finalObj);
    }
    return albums;
}

AlbumInfo AlbumsManage::getBasicInfoCol(SQLite::Statement& sql) {
    AlbumInfo info{};
    info.albumId = sql.getColumn("albumId").getInt64();
    info.hash = DbUtils::getOptCol<juce::String>(sql, "hash");
    info.albumArtists = DbUtils::getOptCol<juce::StringArray>(sql, "albumArtists");
    info.title = sql.getColumn("title").getString();
    return info;
}

// 检查这张专辑是否存在并返回相应的albumId
std::optional<int64_t> AlbumsManage::checkAlbumExist(
    juce::String title,
    juce::StringArray artists,
    std::optional<juce::String> hash
) {
    if (title.isEmpty() || artists.isEmpty()) return std::nullopt;

    try {
        struct AlbumIdAndTitle {
            int64_t albumId;
            std::string title;
        };
        SQLite::Statement searchTitle{
            dbManage.readDb(),
            "SELECT albumId , title FROM albums WHERE title = :title"
        };
        std::vector<AlbumIdAndTitle> titles;
        searchTitle.bind(":title", title.toStdString());
        while (searchTitle.executeStep()) {
            titles.push_back(
                {searchTitle.getColumn("albumId").getInt64(),
                 searchTitle.getColumn("title").getString()}
            );
        }
        struct AlbumIdAndHash {
            int64_t albumId;
            std::string hash;
        };
        std::vector<AlbumIdAndHash> hashs;
        if (hash.has_value()) {

            SQLite::Statement searchHash{
                dbManage.readDb(),
                "SELECT albumId , hash FROM albums WHERE hash = :hash"
            };
            DbUtils::bindOptValue(searchHash, ":hash", hash);

            while (searchHash.executeStep()) {
                hashs.push_back(
                    {searchHash.getColumn("albumId").getInt64(),
                     searchHash.getColumn("hash").getString()}
                );
            }
        }
        struct IdAndScore {
            int64_t albumId;
            double score{1.0};
        };
        std::vector<IdAndScore> scores;

        for (auto& eachTitle : titles) {
            scores.push_back({eachTitle.albumId, 1.0});
        }

        for (auto& eachHash : hashs) {
            auto titleAndHashSameit{
                std::find_if(scores.begin(), scores.end(), [&eachHash](IdAndScore idAndScore) {
                    return eachHash.albumId == idAndScore.albumId;
                })
            };
            if (titleAndHashSameit != scores.end()) {
                // 说明有hash和标题都匹配的项
                titleAndHashSameit->score *= 1.4;

            } else {
                scores.push_back({eachHash.albumId, 1.0});
            }
        }
        if (scores.empty()) return std::nullopt;
        juce::StringArray selectArtistsIds; // 用来搜索的scores中的id号组成的序列
        for (auto& score : scores) {
            selectArtistsIds.add(juce::String(score.albumId));
        }
        std::string selectArtistsStr{"SELECT albumId,albumArtists FROM albums WHERE albumId IN ("};

        selectArtistsStr += selectArtistsIds.joinIntoString(",").toStdString();
        selectArtistsStr += ")";

        SQLite::Statement selectArtistsSql{dbManage.readDb(), selectArtistsStr};

        // 遍历查询结果，计算艺术家相似度，并更新最终得分
        double bestScore = -1.0;
        int64_t bestAlbumId = -1;

        while (selectArtistsSql.executeStep()) {
            int64_t albumId = selectArtistsSql.getColumn("albumId").getInt64();
            juce::String originAlbumArtists =
                selectArtistsSql.getColumn("albumArtists").getString();

            // 解析 JSON 数组
            auto originAlbumArtistsArr =
                Yvar{juce::JSON::fromString(originAlbumArtists)}.toStringArray();

            // 计算艺术家相似度（0.0 ~ 1.0）
            double artistsMatchScore =
                StringUtils::calcArtistsSimlarity(artists, originAlbumArtistsArr);

            // 在 scores 中找到这个 albumId 的基础分
            auto it = std::find_if(scores.begin(), scores.end(), [albumId](const IdAndScore& item) {
                return item.albumId == albumId;
            });

            if (it != scores.end()) {
                // 最终得分 = 基础分 * 艺术家相似度
                double finalScore = it->score * artistsMatchScore;

                if (finalScore > bestScore) {
                    bestScore = finalScore;
                    bestAlbumId = albumId;
                }
            }
        }

        // 6. 返回最佳匹配的 albumId
        if (bestScore < 0.5) return std::nullopt;
        if (bestAlbumId >= 0) return bestAlbumId;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->debug("AlbumsManage::checkAlbumExist output db error:{}", e.what());
    }

    return std::nullopt;
}
std::optional<int64_t> AlbumsManage::insertAlbum(
    juce::String title,
    juce::StringArray artists,
    std::optional<juce::String> hash
) {
    if (title.isEmpty() || artists.isEmpty()) return std::nullopt;
    auto result = dbManage.write([&](SQLite::Database& db) -> std::optional<int64_t> {
        try {
            SQLite::Statement insert{
                db,
                fmt::format(
                    "INSERT INTO albums (title,albumArtists{}) VALUES "
                    "(:title,:albumArtists{})",
                    hash.has_value() ? ",hash" : "",
                    hash.has_value() ? ",:hash" : ""
                )

            };
            insert.bind(":title", title.toStdString());
            insert.bind(":albumArtists", juce::JSON::toString(artists).toStdString());
            DbUtils::bindOptValue(insert, ":hash", hash);
            insert.exec();
            return db.getLastInsertRowid();

        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->error("AlbumsManage::insertAlbum错误:{}", e.what());
            return std::nullopt;
        }
    });
    return result;
}
std::optional<int64_t> AlbumsManage::getAlbumId(int64_t songId) {
    try {
        SQLite::Statement lookup{
            dbManage.readDb(),
            "SELECT albumId FROM songs WHERE songId = :songId"
        };

        lookup.bind(":songId", songId);
        if (lookup.executeStep()) {
            return lookup.getColumn("albumId").getInt64();
        } else {
            return std::nullopt;
        }
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("AlbumManage::getAlbumId发生db error:{}", e.what());
        return std::nullopt;
    }
}
std::vector<int64_t> AlbumsManage::getSongsList(int64_t albumId) {
    SQLite::Statement sql{
        dbManage.readDb(),
        "SELECT songId FROM songs WHERE albumId = :albumId ORDER BY "
        "discNumber,trackNumber,songId "

    };
    sql.bind(":albumId", albumId);
    std::vector<int64_t> songsList;
    while (sql.executeStep()) {
        songsList.push_back(sql.getColumn("songId").getInt64());
    }
    return songsList;
}
juce::var AlbumsManage::getDetailInfo(int64_t albumId) {
    AlbumInfo info{};

    info.songsList = getSongsList(albumId);
    return info.toDetailJson();
}