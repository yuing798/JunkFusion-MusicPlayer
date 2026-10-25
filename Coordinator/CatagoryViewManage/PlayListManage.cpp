#include "./PlayListManage.hpp"
#include "Coordinator/dllManager.hpp"
#include "Model/playlistInfo.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include "Utils/stringUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>

nlohmann::json PlayListManage::getAllPlayListView() {
    try {
        SQLite::Statement selectSql{
            dbManage.readDb(),
            "SELECT id,title,hash,createTime FROM playlists"
        };
        nlohmann::json playlists;
        while (selectSql.executeStep()) {
            PlaylistInfo info{};
            info.id = selectSql.getColumn("id").getInt64();
            info.title = selectSql.getColumn("title").getString();
            info.hash = DbUtils::getOptCol<std::string>(selectSql, "hash");
            info.createTime = selectSql.getColumn("createTime").getString();
            nlohmann::json finalObj{info.toBasicJson()};
            playlists.push_back(finalObj);
        }
        return playlists;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->debug(
            "PlayListManage::getAllPlayListView() throw db error:{}",
            e.what()
        );
        return {};
    }
}
PlaylistInfo PlayListManage::getBasicInfo(int64_t id) {
    try {
        SQLite::Statement selectSql{
            dbManage.readDb(),
            "SELECT title,hash,createTime FROM playlists WHERE id=:id"
        };
        PlaylistInfo info;
        selectSql.bind(":id", id);
        if (selectSql.executeStep()) {
            info.id = id;
            info.title = selectSql.getColumn("title").getString();
            info.hash = DbUtils::getOptCol<std::string>(selectSql, "hash");
            info.createTime = selectSql.getColumn("createTime").getString();
        }
        return info;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->debug(
            "PlayListManage::getBasicInfo(int64_t id) throw db error:{}",
            e.what()
        );
        return {};
    }
}
PlaylistInfo PlayListManage::getDetailInfo(int64_t id) {
    try {

        SQLite::Statement selectList{
            dbManage.readDb(),
            "SELECT songId FROM playlistSongs WHERE playlistId=:id"
        };
        selectList.bind(":id", id);
        PlaylistInfo info{};
        while (selectList.executeStep()) {
            info.songIdList.push_back(selectList.getColumn("songId").getInt64());
        }
        return info;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->debug(
            "PlayListManage::getDetailInfo(int64_t id) throw db error:{}",
            e.what()
        );
        return {};
    }
}
int64_t PlayListManage::createNewPlaylist(std::string title, std::optional<juce::File> coverPath) {
    try {
        spdlog::get(LogDllID)->info("准备创建新歌单:标题{}", title);
        auto res = dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement insertNew1{
                db,
                "INSERT INTO playlists (title,hash) VALUES (:title,:hash)"
            };

            std::optional<std::string> hash;
            if (coverPath.has_value()) {
                hash = coverPathCopyAndConvertToHash(coverPath.value());
            }
            DbUtils::bindOptValue(insertNew1, ":hash", hash);
            insertNew1.bind(":title", title);
            insertNew1.exec();
            auto id = db.getLastInsertRowid();
            spdlog::get(LogDllID)->debug("创建新歌单返回的ID为{}", id);
            return id;
        });
        return res;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "PlayListManage::createNewPlaylist throw db error:{}",
            e.what()
        );
        return -1;
    }
}
void PlayListManage::addNewSongs(int64_t id, std::vector<int64_t> songIds) {
    if (songIds.empty()) return;
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Transaction t{db};
            std::string sqlStr = "INSERT OR IGNORE INTO playlistSongs (playlistId,songId) VALUES ";

            for (size_t i = 0; i < songIds.size(); i++) {
                sqlStr += std::format(" (:playlistId,:songId{}) ", i);
                if (i != songIds.size() - 1) {
                    sqlStr += " , ";
                }
            }
            SQLite::Statement insertNewSong{db, sqlStr};
            insertNewSong.bind(":playlistId", id);
            for (size_t i = 0; i < songIds.size(); i++) {
                insertNewSong.bind(std::format(":songId{}", i), songIds[i]);
            }
            insertNewSong.exec();

            SQLite::Statement checkHashStmt{db, "SELECT hash FROM playlists WHERE id=:id"};
            checkHashStmt.bind(":id", id);
            checkHashStmt.exec();
            if (checkHashStmt.getColumn("hash").isNull()) {
                // 如果这张歌单没有封面，直接把第一首歌的封面交给他
                std::string selectHashStr = "SELECT hash FROM songs WHERE songId IN ( ";
                selectHashStr += StringUtils::vectorToString(songIds, ",");
                selectHashStr += " ) ";
                SQLite::Statement selectHashStmt{db, selectHashStr};
                while (selectHashStmt.executeStep()) {
                    auto hash = DbUtils::getOptCol<std::string>(selectHashStmt, "hash");
                    if (hash.has_value()) {
                        updateCover(id, hash.value());
                        break;
                    }
                }
            }
            t.commit();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::addNewSongs throw db error:{}", e.what());
    }
}
void PlayListManage::removeSongs(int64_t id, std::vector<int64_t> songIds) {
    if (songIds.empty()) return; // 空数组必须提前返回，否则 IN () 语法错误

    try {
        dbManage.write([&](SQLite::Database& db) {
            std::string sqlStr =
                "DELETE FROM playlistSongs WHERE playlistId = :playlistId AND songId IN (";
            for (size_t i = 0; i < songIds.size(); ++i) {
                if (i != 0) sqlStr += ",";
                sqlStr += std::format(":songId{}", i);
            }
            sqlStr += ")";

            SQLite::Statement removeStmt{db, sqlStr};
            removeStmt.bind(":playlistId", id);
            for (size_t i = 0; i < songIds.size(); ++i) {
                removeStmt.bind(std::format(":songId{}", i), songIds[i]);
            }
            removeStmt.exec();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::removeSongs throw db error:{}", e.what());
    }
}
void PlayListManage::deletePlaylist(int64_t id) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement removePlaylistStmt{
                db,
                "DELETE FROM playlists WHERE id = :playlistId"
            };
            removePlaylistStmt.bind(":playlistId", id);
            removePlaylistStmt.exec();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::deletePlaylist throw db error:{}", e.what());
    }
}
void PlayListManage::renamePlayList(int64_t id, std::string title) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement updateStmt{db, "UPDATE playlists SET title=:title WHERE id=:id"};
            updateStmt.bind(":id", id);
            updateStmt.bind(":title", title);
            updateStmt.exec();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::renamePlayList throw db error:{}", e.what());
    }
}
void PlayListManage::updateCover(int64_t id, std::string hash) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement updateStmt{db, "UPDATE playlists SET hash=:hash WHERE id=:id"};
            updateStmt.bind(":id", id);
            updateStmt.bind(":hash", hash);
            updateStmt.exec();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::updateCover throw db error:{}", e.what());
    }
}
std::string PlayListManage::coverPathCopyAndConvertToHash(juce::File coverPath) {
    SHA1 sha1;
    auto hash = sha1(coverPath.loadFileAsString().toStdString());
    juce::File target =
        dllManager::getInstance().getImageDir().getChildFile(hash).getChildFile("original.jpg");
    if (target.existsAsFile()) {
        return hash;
    }
    coverPath.copyFileTo(target);
    return hash;
}
void PlayListManage::updateCover(int64_t id, int64_t songId) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Transaction t{db};
            SQLite::Statement searchStmt{db, "SELECT hash FROM songs WHERE songId=:songId"};
            searchStmt.bind(":songId", songId);
            std::string hash;
            if (searchStmt.executeStep()) {
                hash = searchStmt.getColumn(":hash").getString();
            }
            SQLite::Statement updateStmt{db, "UPDATE playlists SET hash=:hash WHERE id=:id"};
            updateStmt.bind(":hash", hash);
            updateStmt.bind(":id", id);
            updateStmt.exec();
            t.commit();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::updateCover throw db error:{}", e.what());
    }
}