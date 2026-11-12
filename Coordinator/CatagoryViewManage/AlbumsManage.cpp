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
#include <cpr/cpr.h>
#include <cstdint>
#include <fmt/format.h>
#include <optional>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

Yvar AlbumsManage::getAllAlbumsView() {
    // 这个函数在启动时被 FFI 直接调用。异常一旦穿过 FFI 边界整个进程会直接
    // 终止（表现为"闪退"），所以这里必须兜住：库坏了就返回空列表。
    try {
        SQLite::Statement selectSql{
            dbManage.readDb(),
            "SELECT albumId,title,albumArtists,hash FROM albums"
        };
        Yvar albums = Yvar::array();
        while (selectSql.executeStep()) {
            AlbumInfo info{};
            info.albumId = selectSql.getColumn("albumId").getInt64();
            info.title = selectSql.getColumn("title").getString();
            info.albumArtists =
                Yvar::parse(selectSql.getColumn("albumArtists").getString()).toStringArray();
            info.hash = selectSql.getColumn("hash").getString();
            albums.push(info.toBasicJson());
        }
        return albums;
    } catch (const std::exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("AlbumsManage::getAllAlbumsView 失败:{}", e.what());
        return Yvar::array();
    } catch (...) {
        return Yvar::array();
    }
}

AlbumInfo AlbumsManage::getBasicInfoCol(SQLite::Statement& sql) {
    AlbumInfo info{};
    info.albumId = sql.getColumn("albumId").getInt64();
    info.hash = DbUtils::getOptCol<std::string>(sql, "hash");
    info.albumArtists = DbUtils::getOptCol<std::vector<std::string>>(sql, "albumArtists");
    info.title = sql.getColumn("title").getString();
    return info;
}

// 检查这张专辑是否存在并返回相应的albumId
std::optional<int64_t> AlbumsManage::checkAlbumExist(
    std::string title,
    std::vector<std::string> artists,
    std::optional<std::string> hash
) {
    if (title.empty() || artists.empty()) return std::nullopt;

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
        searchTitle.bind(":title", title);
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
        std::vector<std::string> selectArtistsIds; // 用来搜索的scores中的id号组成的序列
        for (auto& score : scores) {
            selectArtistsIds.push_back(std::to_string(score.albumId));
        }
        std::string selectArtistsStr{"SELECT albumId,albumArtists FROM albums WHERE albumId IN ("};

        selectArtistsStr += StringUtils::vectorToString(selectArtistsIds, ",");
        selectArtistsStr += ")";

        SQLite::Statement selectArtistsSql{dbManage.readDb(), selectArtistsStr};

        // 遍历查询结果，计算艺术家相似度，并更新最终得分
        double bestScore = -1.0;
        int64_t bestAlbumId = -1;

        while (selectArtistsSql.executeStep()) {
            int64_t albumId = selectArtistsSql.getColumn("albumId").getInt64();
            std::string originAlbumArtists = selectArtistsSql.getColumn("albumArtists").getString();

            // 解析 JSON 数组
            auto originAlbumArtistsArr = Yvar::parse(originAlbumArtists).toStringArray();

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
    std::string title,
    std::vector<std::string> artists,
    std::optional<std::string> hash
) {
    if (title.empty() || artists.empty()) return std::nullopt;
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
            insert.bind(":title", title);
            insert.bind(":albumArtists", Yvar::fromVector(artists).dump());
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
Yvar AlbumsManage::getDetailInfo(int64_t albumId) {
    AlbumInfo info{};

    info.songsList = getSongsList(albumId);
    return info.toDetailJson();
}

std::optional<int64_t> AlbumsManage::removeAlbumIfEmpty(int64_t albumId) {
    if (albumId <= 0) return std::nullopt;

    return dbManage.write([albumId](SQLite::Database& db) -> std::optional<int64_t> {
        try {
            // 还有歌用着这张专辑就不动它
            SQLite::Statement countSql{db, "SELECT COUNT(*) FROM songs WHERE albumId=:albumId"};
            countSql.bind(":albumId", albumId);
            if (!countSql.executeStep()) return std::nullopt;
            if (countSql.getColumn(0).getInt64() > 0) return std::nullopt;

            SQLite::Statement removeSql{db, "DELETE FROM albums WHERE albumId=:albumId"};
            removeSql.bind(":albumId", albumId);
            removeSql.exec();

            // 一行都没删掉说明这张专辑本来就不在
            if (removeSql.getChanges() == 0) return std::nullopt;

            return albumId;
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->error("AlbumsManage::removeAlbumIfEmpty 失败:{}", e.what());
            return std::nullopt;
        }
    });
}

// ── 联网搜索专辑封面 ────────────────────────────────────────────────────
std::optional<std::string>
AlbumsManage::searchCoverURL(const std::string& term, const std::string& entity) {
    if (auto logger = spdlog::get(LogDllID))
        logger->debug("iTunes 搜索 term: {}, entity: {}", term, entity);

    cpr::Response coverSearch = cpr::Get(
        cpr::Url{"https://itunes.apple.com/search"},
        cpr::Parameters{{"term", term}, {"media", "music"}, {"entity", entity}, {"limit", "1"}},
        cpr::Timeout{15000}
    );

    if (coverSearch.status_code == 200) {
        const Yvar coverSearchRes = Yvar::parse(coverSearch.text);
        if (coverSearchRes.read("resultCount").toInt() > 0) {
            const auto firstResult = coverSearchRes.read("results").read(0);
            return firstResult.read("artworkUrl100").toOptStr();
        }
    } else if (auto logger = spdlog::get(LogDllID)) {
        logger->warn(
            "iTunes 搜索失败 status={} error={}",
            coverSearch.status_code,
            coverSearch.error.message
        );
    }
    return std::nullopt;
}

std::optional<std::string>
AlbumsManage::searchAndSaveCover(int64_t albumId, const juce::File& imageDir) {
    auto logger = spdlog::get(LogDllID);

    // 1. 取专辑名 + 专辑艺术家
    std::string title;
    std::vector<std::string> artists;
    try {
        SQLite::Statement sql{
            dbManage.readDb(),
            "SELECT title, albumArtists FROM albums WHERE albumId=:albumId"
        };
        sql.bind(":albumId", albumId);
        if (!sql.executeStep()) return std::nullopt;

        title = sql.getColumn("title").getString();
        artists = DbUtils::getOptCol<std::vector<std::string>>(sql, "albumArtists")
                      .value_or(std::vector<std::string>{});
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("AlbumsManage::searchAndSaveCover 读专辑失败:{}", e.what());
        return std::nullopt;
    }

    if (title.empty()) return std::nullopt;

    // 2. 主艺术家：群星这类名字发给 Apple 会极大降低命中率，跳过
    std::string primaryArtist;
    for (const auto& artist : artists) {
        if (!StringUtils::isVariousArtists(artist)) {
            primaryArtist = artist;
            break;
        }
    }

    // 3. 先「艺术家 + 专辑名」，搜不到再用纯专辑名兜底
    std::optional<std::string> coverURL;
    if (!primaryArtist.empty()) coverURL = searchCoverURL(primaryArtist + " " + title, "album");

    // 只搜索一次就好，单独搜索专辑名很容易出错
    //  if (!coverURL.has_value()) coverURL = searchCoverURL(title, "album");

    if (!coverURL.has_value()) {
        if (logger) logger->info("搜不到专辑封面:albumId={}, title={}", albumId, title);
        return std::nullopt;
    }

    // 4. 把 100x100 缩略图换成 1000x1000 大图
    const auto pos = coverURL->find("100x100bb.jpg");
    if (pos != std::string::npos) coverURL->replace(pos, 13, "1000x1000bb.jpg");

    // 5. 下载
    cpr::Response imgRes = cpr::Get(cpr::Url{*coverURL}, cpr::Timeout{15000});
    if (imgRes.status_code != 200 || imgRes.text.empty()) {
        if (logger)
            logger->warn(
                "封面下载失败:albumId={}, status={}, error={}",
                albumId,
                imgRes.status_code,
                imgRes.error.message
            );
        return std::nullopt;
    }

    // 6. SHA1 命名落盘：<image>/<hash>/original.jpg
    SHA1 sha1;
    const std::string hash = sha1(imgRes.text.data(), imgRes.text.size());

    juce::File hashImageDir{imageDir.getChildFile(hash)};
    if (!hashImageDir.exists()) hashImageDir.createDirectory();

    juce::File originalFile{hashImageDir.getChildFile("original.jpg")};
    if (!originalFile.existsAsFile()) {
        juce::FileOutputStream outputStream(originalFile);
        if (!outputStream.openedOk()) {
            if (logger)
                logger->error("封面写文件失败:{}", originalFile.getFullPathName().toStdString());
            return std::nullopt;
        }
        outputStream.write(imgRes.text.data(), imgRes.text.size());
        outputStream.flush();
    }

    // 7. 更新 albums.hash
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement update{db, "UPDATE albums SET hash=:hash WHERE albumId=:albumId"};
            update.bind(":hash", hash);
            update.bind(":albumId", albumId);
            update.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("AlbumsManage::searchAndSaveCover 写库失败:{}", e.what());
        return std::nullopt;
    }

    if (logger) logger->info("封面搜索完成:albumId={}, hash={}", albumId, hash);
    return hash;
}