#include "./PluginChainPresetManage.hpp"

#include "Utils/constants.h"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <spdlog/spdlog.h>

Yvar PluginChainPresetManage::getAllPresets() {
    Yvar results = Yvar::array();

    try {
        SQLite::Statement sql{
            mDbManage.readDb(),
            "SELECT id, title, createTime FROM pluginChainPresets ORDER BY id DESC"
        };
        while (sql.executeStep()) {
            Yvar item = Yvar::object();
            item.set("id", static_cast<int64_t>(sql.getColumn("id").getInt64()));
            item.set("title", sql.getColumn("title").getString());
            item.set("createTime", sql.getColumn("createTime").getString());
            results.push(item);
        }
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginChainPresetManage::getAllPresets 失败:{}", e.what());
    }

    return results;
}

Yvar PluginChainPresetManage::getPresetSlots(int64_t id) {
    try {
        SQLite::Statement sql{
            mDbManage.readDb(),
            "SELECT slots FROM pluginChainPresets WHERE id = :id"
        };
        sql.bind(":id", id);
        if (!sql.executeStep()) return Yvar{};

        const auto text = sql.getColumn("slots").getString();
        return Yvar::parse(text);
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginChainPresetManage::getPresetSlots 失败:{}", e.what());
    }
    return Yvar{};
}

int64_t PluginChainPresetManage::createPreset(
    const std::string& title,
    const std::string& slotsJson
) {
    if (title.empty() || slotsJson.empty()) return -1;

    try {
        return mDbManage.write([&](SQLite::Database& db) -> int64_t {
            SQLite::Statement sql{
                db,
                "INSERT INTO pluginChainPresets(title, slots) VALUES(:title, :slots)"
            };
            sql.bind(":title", title);
            sql.bind(":slots", slotsJson);
            sql.exec();
            return db.getLastInsertRowid();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginChainPresetManage::createPreset 失败:{}", e.what());
        return -1;
    }
}

void PluginChainPresetManage::renamePreset(int64_t id, const std::string& title) {
    if (title.empty()) return;
    try {
        mDbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{
                db,
                "UPDATE pluginChainPresets SET title = :title WHERE id = :id"
            };
            sql.bind(":title", title);
            sql.bind(":id", id);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginChainPresetManage::renamePreset 失败:{}", e.what());
    }
}

void PluginChainPresetManage::deletePreset(int64_t id) {
    try {
        mDbManage.write([&](SQLite::Database& db) {
            SQLite::Statement sql{db, "DELETE FROM pluginChainPresets WHERE id = :id"};
            sql.bind(":id", id);
            sql.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("PluginChainPresetManage::deletePreset 失败:{}", e.what());
    }
}
