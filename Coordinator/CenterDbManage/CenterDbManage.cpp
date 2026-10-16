#include "./CenterDbManage.hpp"
#include "Utils/constants.h"
#include "dbTable.hpp"
#include <spdlog/spdlog.h>

CenterDbManage::CenterDbManage(const std::string& path)
    : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {

    try {

        db.exec("PRAGMA busy_timeout = 5000;");
        db.exec("PRAGMA foreign_keys = ON;");
        db.exec("PRAGMA synchronous = NORMAL;");

        db.exec("PRAGMA journal_mode = WAL;");
        db.exec(createSongsTableSQL);
        db.exec(createAlbumsTableSQL);

    } catch (const std::exception& e) {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogDllID);
        if (logger)
            logger->critical(
                "无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}",
                e.what()
            );
    }
}