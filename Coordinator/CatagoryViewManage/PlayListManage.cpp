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

juce::Array<juce::var> PlayListManage::getAllPlayListView() {
    try {
        SQLite::Statement selectSql{
            dbManage.readDb(),
            "SELECT playlistId,title,hash FROM playlists"
        };
        juce::Array<juce::var> playlists;
        while (selectSql.executeStep()) {
            PlaylistInfo info{};
            info.id = selectSql.getColumn("albumId").getInt64();
            info.title = selectSql.getColumn("title").getString();
            info.hash = selectSql.getColumn("hash").getString();
            juce::var finalObj{info.toBasicJson()};
            playlists.add(finalObj);
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
            "SELECT playlistId,title,hash FROM playlists WHERE id=:id"
        };
        PlaylistInfo info;
        selectSql.bind(":id", id);
        if (selectSql.executeStep()) {
            info.id = selectSql.getColumn("albumId").getInt64();
            info.title = selectSql.getColumn("title").getString();
            info.hash = selectSql.getColumn("hash").getString();
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
int64_t PlayListManage::createNewPlaylist(juce::String title, std::optional<juce::File> coverPath) {
    try {
        auto res = dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement insertNew1{
                db,
                "INSERT INTO playlists (title,hash) VALUES (:title,:hash)"
            };

            std::optional<juce::String> hash;
            if (coverPath.has_value()) {
                hash = coverPathCopyAndConvertToHash(coverPath.value());
            }
            DbUtils::bindOptValue(insertNew1, ":hash", hash);
            insertNew1.bind(":title", title.toStdString());
            return db.getLastInsertRowid();
        });
        return res;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "PlayListManage::createNewPlaylist(PlaylistInfo info) throw db error:{}",
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
                    auto hash = DbUtils::getOptCol<juce::String>(selectHashStmt, "hash");
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
void PlayListManage::renamePlayList(int64_t id, juce::String title) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement updateStmt{db, "UPDATE playlists SET title=:title WHERE id=:id"};
            updateStmt.bind(":id", id);
            updateStmt.bind(":title", title.toStdString());
            updateStmt.exec();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::renamePlayList throw db error:{}", e.what());
    }
}
void PlayListManage::updateCover(int64_t id, juce::String hash) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement updateStmt{db, "UPDATE playlists SET hash=:hash WHERE id=:id"};
            updateStmt.bind(":id", id);
            updateStmt.bind(":hash", hash.toStdString());
            updateStmt.exec();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::updateCover throw db error:{}", e.what());
    }
}
juce::String PlayListManage::coverPathCopyAndConvertToHash(juce::File coverPath) {
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
            juce::String hash;
            if (searchStmt.executeStep()) {
                hash = searchStmt.getColumn(":hash").getString();
            }
            SQLite::Statement updateStmt{db, "UPDATE playlists SET hash=:hash WHERE id=:id"};
            updateStmt.bind(":hash", hash.toStdString());
            updateStmt.bind(":id", id);
            updateStmt.exec();
            t.commit();
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("PlayListManage::updateCover throw db error:{}", e.what());
    }
}