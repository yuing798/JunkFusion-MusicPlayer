#include "./songsManage.hpp"
#include "../dllManager.hpp"
#include "Model/PlayInfo.hpp"
#include "Model/SongInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>

#include <memory>
#include <optional>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

std::vector<std::string> SongsManage::getTags(TagLib::PropertyMap& map, const char* key) {
    if (!map.contains(key) || map[key].isEmpty()) {
        return std::vector<std::string>{};
    }

    const TagLib::StringList& rawList = map[key];
    std::vector<std::string> finalItems;

    // 如果 TagLib 提取出了真正的多元素数组 (FLAC 或 ID3v2.4)
    if (rawList.size() > 1) {
        for (const auto& item : rawList) {
            finalItems.push_back(item.to8Bit(true));
        }
    }
    // 如果 TagLib 只提取到了 1 个元素 (旧版 ID3v2.3 硬编码斜杠的情况)
    else {
        std::string rawStr = rawList.front().to8Bit(true);
        std::string tagKey = key;

        // 转为大写，防止调用者传入 "artist" 或 "genre" 导致匹配失败
        std::transform(tagKey.begin(), tagKey.end(), tagKey.begin(), [](unsigned char c) {
            return std::toupper(c);
        });

        // 仅针对 ARTIST 和 GENRE 进行斜杠/分号的二次拆分兼容
        if ((tagKey == "ARTIST" || tagKey == "GENRE") && rawStr != "AC/DC" && // 白名单特例规避
            (rawStr.find('/') != std::string::npos || rawStr.find(';') != std::string::npos)) {
            std::string token;
            for (char ch : rawStr) {
                if (ch == '/' || ch == ';') {
                    // 去除前后空格并保存
                    auto start = token.find_first_not_of(" ");
                    auto end = token.find_last_not_of(" ");
                    if (start != std::string::npos) {
                        finalItems.push_back(token.substr(start, end - start + 1));
                    }
                    token.clear();
                } else {
                    token += ch;
                }
            }
            // 压入最后一个名字
            auto start = token.find_first_not_of(" ");
            auto end = token.find_last_not_of(" ");
            if (start != std::string::npos) {
                finalItems.push_back(token.substr(start, end - start + 1));
            }
        } else {
            finalItems.push_back(rawStr);
        }
    }

    return finalItems;
}

std::optional<int> SongsManage::getTagInt(TagLib::PropertyMap& map, const char* key) {
    // auto tags{getTags(map, key)};
    if (!map.contains(key) || map[key].isEmpty()) {
        return std::nullopt;
    }
    auto& rawList{map[key]};

    try {
        if (rawList.isEmpty()) {
            return std::nullopt;
        }
        return std::stoi(rawList[0].to8Bit());
    } // std::stoi 解析 "2023-05-12" 会自动提取 2023
    catch (...) {
        return std::nullopt;
    }
}

SongInfo SongsManage::getBasicInfoCol(SQLite::Statement& query) {
    SongInfo info{};

    info.songId = query.getColumn("songId").getInt64();
    info.duration = query.getColumn("duration").getDouble();

    // ── 标签 ──
    info.title = query.getColumn("title").getString();
    info.artists = DbUtils::getOptCol<std::vector<std::string>>(query, "artists");
    info.album = DbUtils::getOptCol<std::string>(query, "album");

    // ── 用户信息 ──
    info.isMyLike = query.getColumn("isMyLike").getInt() != 0;
    info.playNum = query.getColumn("playNum").getInt();
    info.hash = DbUtils::getOptCol<std::string>(query, "hash");
    return info;
}
SongInfo SongsManage::getDetailInfoCol(SQLite::Statement& query) {
    SongInfo info{};
    info.trackNumber = DbUtils::getOptCol<int>(query, "trackNumber");
    info.discNumber = DbUtils::getOptCol<int>(query, "discNumber");
    // ── FFmpeg 解码层 ──

    info.bitRate = query.getColumn("bitRate").getInt64();
    info.bitDepth = query.getColumn("bitDepth").getInt();
    info.sampleRate = query.getColumn("sampleRate").getInt();

    auto channelLayoutMask = query.getColumn("channelLayoutMask").getInt64();
    if (channelLayoutMask != 0) {

        AVChannelLayout layout;
        av_channel_layout_from_mask(&layout, channelLayoutMask);
        char layoutBuffer[64] = {0};
        av_channel_layout_describe(&layout, layoutBuffer, sizeof(layoutBuffer));
        info.channelLayout = std::string(layoutBuffer);
    }
    info.codecName = DbUtils::getOptCol<std::string>(query, "codecName");

    // ── AI 分析 ──
    info.bpm = DbUtils::getOptCol<int>(query, "bpm");
    info.key = DbUtils::getOptCol<std::string>(query, "key");

    info.timeDomainSpec = DbUtils::getBlob<double>(query, "timeDomainSpec");
    return info;
}
SongInfo SongsManage::getBasicSongInfo(int64_t songId) {
    try {
        SQLite::Statement query{
            dbManage.readDb(),
            R"(
                SELECT songId, 
                duration, title, artists, album, 
                isMyLike, playNum, hash
                FROM songs WHERE songId = :songId
            )"
        };
        query.bind(":songId", songId);

        SongInfo info;
        if (query.executeStep()) {
            info = getBasicInfoCol(query);
        } else {
            throw SQLite::Exception{"无法根据id找到歌曲信息:路径:" + std::to_string(songId)};
        }
        return info;

    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->info(
            "SongsManage::getBasicSongInfo(int64_t songId)发生数据库错误:{}",
            e.what()
        );
        return {};
    }
}

SongInfo SongsManage::getDetailSongInfo(int64_t songId) {
    try {
        SQLite::Statement query{
            dbManage.readDb(),
            R"(
                SELECT 
                trackNumber,discNumber,
                bitRate, bitDepth, sampleRate, channelLayoutMask,  codecName,
                bpm, key, 
                timeDomainSpec 
                FROM songs WHERE songId = :songId
            )"
        };
        query.bind(":songId", songId);

        SongInfo info;
        if (query.executeStep()) {
            info = getDetailInfoCol(query);
        } else {
            throw SQLite::Exception{"无法根据id找到歌曲信息:路径:" + std::to_string(songId)};
        }
        return info;

    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->info(
            "SongsManage::getDetailSongInfo(int64_t songId)发生数据库错误:{}",
            e.what()
        );
        return {};
    }
}
Yvar SongsManage::getAllSongsBasicInfo() {
    Yvar results = Yvar::array();

    try {

        SQLite::Statement query(dbManage.readDb(), R"(
            SELECT songId, 
            duration, title, artists, album, 
            isMyLike, playNum, hash
            FROM songs ORDER BY songId
        )");

        while (query.executeStep()) {

            results.push(getBasicInfoCol(query).toBasicJson());
        }
    } catch (const SQLite::Exception& e) {
        auto logger = spdlog::get(LogDllID);
        logger->error("getAllSongsBasicInfo发生失败:{}", e.what());
    }

    return results;
}

PlayInfo SongsManage::getPlayInfo(int64_t songId) {
    spdlog::get(LogDllID)->debug("开始根据id搜索歌曲信息");
    PlayInfo info{};
    try {
        SQLite::Statement sql(
            dbManage.readDb(),
            R"(
            SELECT filePath, duration, title, artists 
            FROM songs WHERE songId = :songId)"
        );
        sql.bind(":songId", songId);
        if (sql.executeStep()) {
            info.path = sql.getColumn("filePath").getString();
            info.duration = sql.getColumn("duration").getDouble();
            info.title = sql.getColumn("title").getString();
            Yvar originArtists = Yvar::parse(sql.getColumn("artists").getString());
            info.artists = originArtists.toStringArray();

        } else {
            throw SQLite::Exception{"我草他妈的找不着:" + std::to_string(songId)};
        }
        spdlog::get(LogDllID)->debug("成功获取:id:{},路径:{}", songId, info.path);
        return info;
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogDllID)};
        logger->error("SongsManage::getPlayInfoBySongId(int64_t songId)发生错误:{}", e.what());
        return {};
    }
}

void SongsManage::reverseMyLike(int64_t id) {

    dbManage.write([id](SQLite::Database& db) {
        try {
            SQLite::Statement sql(
                db,
                "UPDATE songs SET isMyLike = 1 - isMyLike WHERE songId = :songId"
            );
            sql.bind(":songId", id);

            sql.exec();
        } catch (const SQLite::Exception& e) {
            auto logger{spdlog::get(LogDllID)};
            logger->error("[我喜欢]状态更新失败，请重试:{}", e.what());
        }
    });
    dllManager::getInstance().notifySongUpdated(id);
}
void SongsManage::updatePlayCount(std::string path) {
    int64_t updatedSongId{0};

    dbManage.write([&](SQLite::Database& db) {
        try {
            SQLite::Statement sql{
                db,
                "UPDATE songs SET playNum = (playNum + 1) WHERE filePath = :filePath RETURNING "
                "songId"
            };

            sql.bind(":filePath", path);
            if (sql.executeStep()) {
                updatedSongId = sql.getColumn("songId").getInt64();
            }
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug(
                "SongsManage::updatePlayCount(std::string path)发生错误:{}",
                e.what()
            );
        }
    });

    // 通知放到写事务外面：它内部还要读一次库拿最新信息
    if (updatedSongId > 0) dllManager::getInstance().notifySongUpdated(updatedSongId);
}

void SongsManage::updateLastPlayTime(int64_t songId) {
    if (songId <= 0) return;

    dbManage.write([&](SQLite::Database& db) {
        try {
            SQLite::Statement sql{
                db,
                "UPDATE songs SET lastPlayTime = datetime('now','localtime') WHERE "
                "songId = :songId"
            };
            sql.bind(":songId", songId);
            sql.exec();
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug("SongsManage::updateLastPlayTime 发生错误:{}", e.what());
        }
    });
}

Yvar SongsManage::getSingleImportSongs() {
    auto songIdList = Yvar::array();
    try {
        SQLite::Statement sql(dbManage.readDb(), "SELECT songId FROM songs WHERE isSingleFile=1");

        while (sql.executeStep()) {
            songIdList.push_back(sql.getColumn("songId").getInt64());
        }
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogDllID)};
        logger->error("SongsManage::getSingleImportSongs()发生错误:{}", e.what());
    }
    return songIdList;
}