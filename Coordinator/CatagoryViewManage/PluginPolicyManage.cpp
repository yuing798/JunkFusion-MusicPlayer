#include "./PluginPolicyManage.hpp"

#include "Utils/constants.h"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <spdlog/spdlog.h>

Yvar PluginPolicyManage::getAll() {
    Yvar out = Yvar::object();
    Yvar black = Yvar::array();
    Yvar white = Yvar::array();

    try {
        {
            SQLite::Statement sql{
                mDbManage.readDb(),
                "SELECT identifier, filePath, reason, failureCount, lastFailedAt "
                "FROM pluginBlacklist ORDER BY lastFailedAt DESC"
            };
            while (sql.executeStep()) {
                Yvar item = Yvar::object();
                item.set("identifier", sql.getColumn("identifier").getString());
                item.set("filePath", sql.getColumn("filePath").getString());
                item.set("reason", sql.getColumn("reason").getString());
                item.set("failureCount", sql.getColumn("failureCount").getInt());
                item.set("lastFailedAt", sql.getColumn("lastFailedAt").getString());
                black.push(item);
            }
        }
        {
            SQLite::Statement sql{
                mDbManage.readDb(),
                "SELECT identifier, filePath, note, createdAt FROM pluginWhitelist "
                "ORDER BY createdAt DESC"
            };
            while (sql.executeStep()) {
                Yvar item = Yvar::object();
                item.set("identifier", sql.getColumn("identifier").getString());
                item.set("filePath", sql.getColumn("filePath").getString());
                item.set("note", sql.getColumn("note").getString());
                item.set("createdAt", sql.getColumn("createdAt").getString());
                white.push(item);
            }
        }
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginPolicyManage::getAll 失败:{}", e.what());
    }

    out.set("blacklist", black);
    out.set("whitelist", white);
    return out;
}

std::vector<std::string> PluginPolicyManage::getBlacklisted() {
    std::vector<std::string> ids;

    try {
        SQLite::Statement sql{mDbManage.readDb(), "SELECT identifier FROM pluginBlacklist"};
        while (sql.executeStep()) ids.push_back(sql.getColumn("identifier").getString());
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginPolicyManage::getBlacklisted 失败:{}", e.what());
    }

    return ids;
}

bool PluginPolicyManage::isWhitelisted(const std::string& identifier) {
    if (identifier.empty()) return false;

    try {
        SQLite::Statement sql{
            mDbManage.readDb(),
            "SELECT 1 FROM pluginWhitelist WHERE identifier = :id LIMIT 1"
        };
        sql.bind(":id", identifier);
        return sql.executeStep();
    } catch (const SQLite::Exception&) {
        return false; // 读不到就当没白名单（保守：仍然会过黑名单）
    }
}

void PluginPolicyManage::ban(
    const std::string& identifier,
    const std::string& filePath,
    const std::string& reason
) {
    if (identifier.empty()) return;

    try {
        mDbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{
                db,
                "INSERT INTO pluginBlacklist(identifier, filePath, reason, failureCount, lastFailedAt) "
                "VALUES(:id, :path, :reason, 1, datetime('now','localtime')) "
                "ON CONFLICT(identifier) DO UPDATE SET "
                "filePath = excluded.filePath, "
                "reason = excluded.reason, "
                "failureCount = failureCount + 1, "
                "lastFailedAt = excluded.lastFailedAt"
            };
            sql.bind(":id", identifier);
            sql.bind(":path", filePath);
            sql.bind(":reason", reason);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginPolicyManage::ban 失败:{}", e.what());
    }
}

void PluginPolicyManage::clearBlack(const std::string& identifier) {
    if (identifier.empty()) return;
    try {
        mDbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{
                db,
                "DELETE FROM pluginBlacklist WHERE identifier = :id"
            };
            sql.bind(":id", identifier);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginPolicyManage::clearBlack 失败:{}", e.what());
    }
}

void PluginPolicyManage::addWhite(const std::string& identifier, const std::string& filePath) {
    if (identifier.empty()) return;
    try {
        mDbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{
                db,
                "INSERT INTO pluginWhitelist(identifier, filePath, note, createdAt) "
                "VALUES(:id, :path, NULL, datetime('now','localtime')) "
                "ON CONFLICT(identifier) DO NOTHING"
            };
            sql.bind(":id", identifier);
            sql.bind(":path", filePath);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginPolicyManage::addWhite 失败:{}", e.what());
    }
}

void PluginPolicyManage::removeWhite(const std::string& identifier) {
    if (identifier.empty()) return;
    try {
        mDbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{
                db,
                "DELETE FROM pluginWhitelist WHERE identifier = :id"
            };
            sql.bind(":id", identifier);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID)) logger->warn("removeWhite 失败:{}", e.what());
    }
}
