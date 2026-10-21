#include "./PlayListManage.hpp"
#include "Model/playlistInfo.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <spdlog/spdlog.h>

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
PlaylistInfo PlayListManage::getCompleteInfo(int64_t id) {
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
        SQLite::Statement selectList{
            dbManage.readDb(),
            "SELECT songId FROM songs WHERE playlistId=:id"
        };
        selectList.bind(":id", id);
        while (selectList.executeStep()) {
            info.songIdList.push_back(selectList.getColumn("songId").getInt64());
        }
        return info;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->debug(
            "PlayListManage::getCompleteInfo(int64_t id) throw db error:{}",
            e.what()
        );
        return {};
    }
}
bool PlayListManage::createNewPlaylist(PlaylistInfo info) {
    try {
        auto res = dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement insertNew1{
                db,
                "INSERT INTO playlists (title,hash) VALUES (:title,:hash)"
            };
            DbUtils::bindOptValue(insertNew1, ":hash", info.hash);
            insertNew1.bind(":title", info.title.toStdString());
            if (!insertNew1.exec()) {
                return false;
            }
            if (info.songIdList.empty()) {
                return true;
            }
            auto playlistId = db.getLastInsertRowid();
            std::string updateSql = "UPDATE songs SET playlistId=:playlistId WHERE songId IN (";
            updateSql += ConvertUtils::vectorToStringArray(info.songIdList)
                             .joinIntoString(",")
                             .toStdString();
            updateSql += ")";
            SQLite::Statement updateNew{db, updateSql};
            updateNew.bind(":playlistId", playlistId);
            if (!updateNew.exec()) {
                return false;
            } else {
                return true;
            }
        });
        return res;
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "PlayListManage::createNewPlaylist(PlaylistInfo info) throw db error:{}",
            e.what()
        );
        return false;
    }
}