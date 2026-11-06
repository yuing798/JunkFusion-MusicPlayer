#include "./EqPresetManage.hpp"
#include "Utils/constants.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <spdlog/spdlog.h>
#include <string>

Yvar EqPresetManage::getAllPresets() {
    Yvar presets = Yvar::array();

    try {
        SQLite::Statement selectSql{
            dbManage.readDb(),
            "SELECT id, title, createTime FROM eqPresets ORDER BY id"
        };

        while (selectSql.executeStep()) {
            Yvar item = Yvar::object();
            item.set("id", selectSql.getColumn("id").getInt64());
            item.set("title", selectSql.getColumn("title").getString());
            item.set("createTime", selectSql.getColumn("createTime").getString());
            presets.push(std::move(item));
        }
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("EqPresetManage::getAllPresets 出错:{}", e.what());
    }

    return presets;
}

Yvar EqPresetManage::getPresetParams(int64_t id) {
    try {
        SQLite::Statement selectSql{dbManage.readDb(), "SELECT params FROM eqPresets WHERE id=:id"};
        selectSql.bind(":id", id);

        if (selectSql.executeStep()) {
            // 解析失败会得到空值，调用方按"没有这个预设"处理即可
            return Yvar::parse(selectSql.getColumn("params").getString());
        }
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("EqPresetManage::getPresetParams 出错:{}", e.what());
    }

    return Yvar{};
}

int64_t EqPresetManage::createPreset(const std::string& title, const std::string& paramsText) {
    try {
        return dbManage.write([&](SQLite::Database& db) -> int64_t {
            SQLite::Statement insert{
                db,
                "INSERT INTO eqPresets (title, params) VALUES (:title, :params)"
            };
            insert.bind(":title", title);
            insert.bind(":params", paramsText);
            insert.exec();
            return static_cast<int64_t>(db.getLastInsertRowid());
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("EqPresetManage::createPreset 出错:{}", e.what());
        return -1;
    }
}

void EqPresetManage::renamePreset(int64_t id, const std::string& title) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement update{db, "UPDATE eqPresets SET title=:title WHERE id=:id"};
            update.bind(":id", id);
            update.bind(":title", title);
            update.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("EqPresetManage::renamePreset 出错:{}", e.what());
    }
}

void EqPresetManage::deletePreset(int64_t id) {
    try {
        dbManage.write([&](SQLite::Database& db) {
            SQLite::Statement remove{db, "DELETE FROM eqPresets WHERE id=:id"};
            remove.bind(":id", id);
            remove.exec();
        });
    } catch (const SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("EqPresetManage::deletePreset 出错:{}", e.what());
    }
}
