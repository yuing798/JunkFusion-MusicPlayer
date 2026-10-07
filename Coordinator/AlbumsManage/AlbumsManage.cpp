#include "./AlbumsManage.hpp"
#include "Coordinator/DbTable/dbTable.hpp"
#include "Utils/constants.h"
#include <spdlog/spdlog.h>

AlbumsManage::AlbumsManage(SQLite::Database* db) : db(db) {

    try {
        db->exec(createAlbumsTableSQL);
    } catch (const std::exception& e) {
        auto logger = spdlog::get(LogDllID);
        if (logger)
            logger->critical(
                "无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}",
                e.what()
            );
    }
}
bool AlbumsManage::checkAlbum(juce::String title, juce::StringArray artists, juce::String hash) {}
bool AlbumsManage::insertAlbum(juce::String title, juce::StringArray artists, juce::String hash) {}