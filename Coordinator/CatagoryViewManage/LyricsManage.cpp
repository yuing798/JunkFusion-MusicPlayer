#include "./LyricsManage.hpp"

#include "Utils/constants.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <spdlog/spdlog.h>

void LyricsManage::upsert(
    int64_t songId,
    const std::string& kind,
    const std::string& format,
    const std::string& content,
    const std::string& sourcePath,
    int64_t mtime
) {
    if (songId <= 0 || content.empty()) return;

    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{
                db,
                "INSERT INTO lyrics(songId, kind, format, content, sourcePath, mtime) "
                "VALUES(:songId, :kind, :format, :content, :sourcePath, :mtime) "
                "ON CONFLICT(songId, kind) DO UPDATE SET "
                "format=excluded.format, content=excluded.content, "
                "sourcePath=excluded.sourcePath, mtime=excluded.mtime"
            };
            sql.bind(":songId", songId);
            sql.bind(":kind", kind);
            sql.bind(":format", format);
            sql.bind(":content", content);
            sql.bind(":sourcePath", sourcePath);
            sql.bind(":mtime", mtime);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("LyricsManage::upsert 失败 songId={}:{}", songId, e.what());
    }
}

std::string LyricsManage::readContent(int64_t songId, const std::string& kind) {
    try {
        SQLite::Statement sql{
            dbManage.readDb(),
            "SELECT content FROM lyrics WHERE songId=:songId AND kind=:kind"
        };
        sql.bind(":songId", songId);
        sql.bind(":kind", kind);
        if (sql.executeStep()) return sql.getColumn("content").getString();
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("LyricsManage::readContent 失败 songId={}:{}", songId, e.what());
    }
    return {};
}

std::optional<LyricDocument> LyricsManage::get(int64_t songId) {
    const std::string original = readContent(songId, kKindOriginal);
    if (original.empty()) return std::nullopt;

    const std::string translation = readContent(songId, kKindTranslation);
    return LyricParser::parse(original, translation);
}

bool LyricsManage::hasLyrics(int64_t songId) {
    try {
        SQLite::Statement sql{
            dbManage.readDb(),
            "SELECT 1 FROM lyrics WHERE songId=:songId AND kind=:kind LIMIT 1"
        };
        sql.bind(":songId", songId);
        sql.bind(":kind", kKindOriginal);
        return sql.executeStep();
    } catch (const SQLite::Exception&) {
        return false;
    }
}

void LyricsManage::remove(int64_t songId) {
    if (songId <= 0) return;

    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{db, "DELETE FROM lyrics WHERE songId=:songId"};
            sql.bind(":songId", songId);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("LyricsManage::remove 失败 songId={}:{}", songId, e.what());
    }
}
