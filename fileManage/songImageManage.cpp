#include "./songImageManage.hpp"
#include "constants.h"
#include "songsModel.hpp"
#include <spdlog/spdlog.h>

SongImageManager::SongImageManager()
:db(nullptr){
    auto dbFile = databaseDirId.getChildFile("songImage.db");

    try
    {
        db = std::make_unique<SQLite::Database>(
            dbFile.getFullPathName().toStdString(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        createTable();
    }
    catch (const std::exception& e)
    {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogSchedulerID);
        if (logger) logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}", e.what());
    }
}
void SongImageManager::createTable(){
    db->exec(createSongImageTableSQL);
}
