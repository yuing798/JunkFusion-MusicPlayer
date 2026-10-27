#include "./SongFolderManage.hpp"
#include "Macro/FileMacro.hpp"
#include "Macro/coordinatorMacro.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/fileUtils.hpp"
#include "Utils/stringUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstddef>
#include <filesystem>
#include <format>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

Yvar SongFolderManage::getAllRootDirView() {
    Yvar folders = Yvar::array();
    try {
        SQLite::Statement selectSql{
            dbManage.readDb(),
            "SELECT dir FROM songFolders WHERE isRoot = 1"
        };

        while (selectSql.executeStep()) {
            folders.push_back(selectSql.getColumn("dir").getString());
        }
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "SongFolderManage::getAllRootDirWiew throw db error:{}",
            e.what()
        );
    }
    return folders;
}
Yvar SongFolderManage::getChildDirAndSongIdList(const juce::File& parentPath) {
    auto obj = Yvar::object();
    auto dirArr = Yvar::array();
    auto songIdList = Yvar::array();
    auto children = parentPath.findChildFiles(juce::File::findFilesAndDirectories, false);
    std::vector<std::string> songPathVec;

    // 需要做重扫描的文件夹
    // auto dirsNeedRescan = checkDirUpdateRecursion(parentPath);

    for (const auto& child : children) {
        if (child.isDirectory()) {
            dirArr.push_back(child.getFullPathName().toStdString());
        } else {
            auto songPath = child.getFullPathName().toStdString();
            auto it = mPathToIdMap.find(songPath);
            if (it != mPathToIdMap.end()) {
                songIdList.push_back(it->second);
            } else {
                songPathVec.push_back(songPath);
            }
        }
    }
    if (!songPathVec.empty()) {
        try {
            std::string searchSql = "SELECT songId,filePath FROM songs WHERE filePath IN (";
            for (size_t i = 0; i < songPathVec.size(); i++) {
                searchSql += ":path" + std::to_string(i);
                if (i != songPathVec.size() - 1) {
                    searchSql += ",";
                }
            }
            searchSql += ")";

            SQLite::Statement searchStmt{dbManage.readDb(), searchSql};
            for (size_t i = 0; i < songPathVec.size(); i++) {
                std::string pathBing = ":path" + std::to_string(i);
                searchStmt.bind(pathBing, songPathVec[i]);
            }
            while (searchStmt.executeStep()) {
                auto id = searchStmt.getColumn("songId").getInt64();
                songIdList.push_back(id);
                auto path = searchStmt.getColumn("filePath").getString();
                mPathToIdMap[path] = id;
            }
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->error(
                "SongFolderManage::getChildDirAndSongIdList throw db error:{}",
                e.what()
            );
        }
    }
    obj.set(CoordinatorMacro::songIdList, songIdList);
    obj.set(FileMacro::dirArr, dirArr);
    return obj;
}
bool SongFolderManage::isDirHasImport(const juce::File& dir) {
    try {
        SQLite::Statement checkStmt{
            dbManage.readDb(),
            "SELECT 1 FROM songFolders WHERE dir = :dir LIMIT 1"
        };
        checkStmt.bind(":dir", dir.getFullPathName().toStdString());
        return checkStmt.executeStep();
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "SongFolderManage::isDirHasImport throw db error:{}",
            e.what()
        );
        return false;
    }
}
bool SongFolderManage::isDirRoot(const juce::File& dir) {
    try {
        SQLite::Statement checkStmt{
            dbManage.readDb(),
            "SELECT isRoot FROM songFolders WHERE dir = :dir "
        };
        checkStmt.bind(":dir", dir.getFullPathName().toStdString());

        return checkStmt.executeStep();
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("SongFolderManage::isDirRoot throw db error:{}", e.what());
        return false;
    }
}
void SongFolderManage::insertFolderToTable(const juce::File& dir, bool isRoot) {
    try {
        SQLite::Statement insertStmt{
            dbManage.readDb(),
            "INSERT INTO songFolders (dir,isRoot,lastModifiedTime) VALUES "
            "(:dir,:isRoot,:lastModifiedTime)"
        };
        insertStmt.bind(":dir", dir.getFullPathName().toStdString());
        insertStmt.bind(":isRoot", isRoot);
        insertStmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(dir));
        insertStmt.exec();
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "SongFolderManage::insertFolderToTable throw db error:{}",
            e.what()
        );
    }
}
bool SongFolderManage::checkSingleDirUpdate(const juce::File& dir) {
    try {
        SQLite::Statement checkStmt{
            dbManage.readDb(),
            "SELECT lastModifiedTime FROM songFolders WHERE dir = :dir "
        };
        checkStmt.bind(":dir", dir.getFullPathName().toStdString());
        if (checkStmt.executeStep()) {
            // 顺便更新一下
            auto lastModifiedTime = checkStmt.getColumn("lastModifiedTime").getString();
            auto newModifiedTime = FileUtils::getFileLastModifiedTime(dir);
            if (lastModifiedTime != newModifiedTime) {
                // 时间不相等，执行更新逻辑
                dbManage.write([&](SQLite::Database& db) {
                    SQLite::Statement updateTimeStmt{
                        db,
                        "UPDATE songFolders SET lastModifiedTime=:lastModifiedTime WHERE dir=:dir"
                    };
                    updateTimeStmt.bind(":dir", dir.getFullPathName().toStdString());
                    updateTimeStmt.bind(":lastModifiedTime", newModifiedTime);
                    updateTimeStmt.exec();
                });
                return true;
            } else {
                return false;
            }
        }
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("SongFolderManage::isDirRoot throw db error:{}", e.what());
        return false;
    }
    return false;
}

std::vector<juce::File> SongFolderManage::checkDirUpdateRecursion(const juce::File& dir) {
    std::vector<juce::File> res;
    if (checkSingleDirUpdate(dir)) {
        res.push_back(dir);
        auto fileArr = dir.findChildFiles(juce::File::findDirectories, true);
        for (auto& child : fileArr) {
            if (checkSingleDirUpdate(child)) {
                res.push_back(child);
            }
        }
    }
    return res;
}