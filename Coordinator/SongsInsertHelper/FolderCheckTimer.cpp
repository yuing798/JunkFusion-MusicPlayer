#include "./FolderCheckTimer.hpp"
#include "FolderCheckTimer.hpp"
#include "SongImport.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include "Utils/fileUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <algorithm>
#include <cstddef>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

FolderCheckTimer::FolderCheckTimer(DbContext& dbContext, SongImport& songImport)
    : mDbContext(dbContext), mSongImport(songImport) {
    startTimer(60000);
}
void FolderCheckTimer::timerCallback() {
    auto compareRes = compareAllDirState();
    if (compareRes.empty()) return;
    mDbContext.dbManage.write([&](SQLite::Database& db) {
        SQLite::Transaction t{db};
        for (auto& [k, v] : compareRes) {
            if (v.sqlState == SqlState::Add) {

                SQLite::Statement addStmt{
                    db,
                    "INSERT INTO songFolders (dir,isRoot,lastModifiedTime) VALUES "
                    "(:dir,:isRoot,:lastModifiedTime)"
                };
                addStmt.bind(":dir", k.getFullPathName().toStdString());
                addStmt.bind(":isRoot", v.isRoot);
                addStmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(k));
                addStmt.exec();
            } else if (v.sqlState == SqlState::Delete) {
                SQLite::Statement deleteStmt{db, "DELETE FROM songFolders WHERE dir=:dir"};
                deleteStmt.bind(":dir", k.getFullPathName().toStdString());
                deleteStmt.executeStep();
                // songs表中的行会级联删除的，不需要自己管
            } else if (v.sqlState == SqlState::Update) {
                SQLite::Statement updateStmt{
                    db,
                    "UPDATE songFolders SET lastModifiedTime=:lastModifiedTime WHERE dir=:dir"
                };
                updateStmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(k));
                updateStmt.bind(":dir", k.getFullPathName().toStdString());
                updateStmt.exec();
            }
        }
        t.commit();

        for (auto& [k, v] : compareRes) {
            if (v.sqlState == SqlState::Add) {
                auto children = k.findChildFiles(juce::File::findFiles, false);
                for (auto& child : children) {

                    // 新文件夹的歌曲是全部没有导入的
                    mSongImport
                        .importSingleSong(mSongImport.checkSongHasImport(child), child, false);
                }

            } else if (v.sqlState == SqlState::Update) {
                // 应该要获取导入结果，然后把导入结果和表中(当前)表中的内容进行比对

                std::vector<juce::File> songsInThisDirInTable;
                std::vector<juce::File> realSongsInThisDir;

                SQLite::Statement getSongInThisDirStmt{
                    db,
                    "SELECT filePath FROM songs WHERE songFolderId IN ( SELECT id FROM songFolders "
                    "WHERE dir=:dir )"
                };

                getSongInThisDirStmt.bind(":dir", k.getFullPathName().toStdString());
                while (getSongInThisDirStmt.executeStep()) {
                    auto filePath =
                        juce::File{getSongInThisDirStmt.getColumn("filePath").getString()};

                    songsInThisDirInTable.push_back(filePath);
                }

                auto realFilesInThisDir = k.findChildFiles(juce::File::findFiles, false);
                for (auto& realFile : realFilesInThisDir) {
                    auto checkRes = mSongImport.checkSongHasImport(k);
                    if (checkRes == SongHasImportRes::HasImport) {
                        realSongsInThisDir.push_back(realFile);
                        continue;
                    }
                    auto importRes = mSongImport.importSingleSong(checkRes, k, false);

                    if (importRes) {
                        realSongsInThisDir.push_back(realFile);
                        songsInThisDirInTable.push_back(realFile);
                    }
                }

                for (auto& song : songsInThisDirInTable) {
                    auto it = std::find(realSongsInThisDir.begin(), realSongsInThisDir.end(), song);
                    if (it == realSongsInThisDir.end()) {
                        // 表中的文件在检查完成后已经没了，删除即可
                        SQLite::Statement deleteStmt{
                            db,
                            "DELETE FROM songs WHERE filePath=:filePath"
                        };
                        deleteStmt.bind(":filePath", song.getFullPathName().toStdString());
                        deleteStmt.exec();
                    }
                } // 文件夹中有的但是表里面没有的已经加入了，现在只需要删除就行
            }
        }
    });
}
std::unordered_map<juce::File, FolderCheckTimer::DirState> FolderCheckTimer::compareAllDirState() {

    std::unordered_map<juce::File, FolderCheckTimer::DirState> res;

    try {
        // 表中的文件夹
        auto rootDirsInTable = mDbContext.songFolderManage.getAllRootDirOrChildDir(true);
        auto childDirsInTable = mDbContext.songFolderManage.getAllRootDirOrChildDir(false);
        std::vector<juce::File> rootDirsReal;         // 真实的根目录
        std::vector<juce::File> childDirsReal;        // 真实的子目录
        std::vector<juce::File> dirNeedToCheckUpdate; // 需要检查是否更新的文件夹

        // 检查表中的文件夹是否存在，检查是否应该删除
        // 先检查根目录
        for (auto& dir : rootDirsInTable) {

            if (!dir.exists()) {
                res[dir] = {SqlState::Delete, true};
            } else {
                rootDirsReal.push_back(dir);
            }

            SQLite::Statement checkNullStmt{
                mDbContext.dbManage.readDb(),
                "SELECT lastModifiedTime FROM songFolders WHERE dir=:dir"
            };
            checkNullStmt.bind(":dir", dir.getFullPathName().toStdString());
            if (checkNullStmt.executeStep()) {
            }
        }
        // 再检查子目录
        for (auto& dir : childDirsInTable) {
            if (!dir.exists()) {
                res[dir] = {SqlState::Delete, false};
            } // 真实的子目录直接通过迭代获取
        }

        // 这里拿到的就是磁盘真实根目录了,这里获取所有真实的子目录
        for (auto& rootDir : rootDirsReal) {
            auto childDirs = rootDir.findChildFiles(juce::File::findDirectories, true);
            for (auto& childDir : childDirs) {
                childDirsReal.push_back(childDir);
            }
        }

        for (auto& realDir : childDirsReal) {
            auto it = std::find(childDirsInTable.begin(), childDirsInTable.end(), realDir);
            if (it != childDirsInTable.end()) {
                dirNeedToCheckUpdate.push_back(realDir);
            } else {
                res[realDir] = {SqlState::Add, false};
            }
        }

        // 这里检查是否需要更新
        std::string checkUpdateStmtArr =
            DbUtils::addPrefixAndIntoVectorString(":dir", (int)dirNeedToCheckUpdate.size());

        SQLite::Statement checkUpdateStmt{
            mDbContext.dbManage.readDb(),
            "SELECT lastModifiedTime,dir,isRoot FROM songFolders WHERE dir IN " + checkUpdateStmtArr
        };
        for (size_t i = 0; i < dirNeedToCheckUpdate.size(); i++) {
            checkUpdateStmt.bind(
                ":dir" + std::to_string(i),
                dirNeedToCheckUpdate[i].getFullPathName().toStdString()
            );
        }
        while (checkUpdateStmt.executeStep()) {
            auto lastModifiedTime = checkUpdateStmt.getColumn("lastModifiedTime").getString();
            auto dir = checkUpdateStmt.getColumn("dir").getString();
            auto isRoot = checkUpdateStmt.getColumn("isRoot").getInt();
            auto newModifiedTime = FileUtils::getFileLastModifiedTime(juce::File{dir});
            if (lastModifiedTime != newModifiedTime) {
                res[juce::File{dir}] = {SqlState::Update, isRoot ? true : false};
            }
        }

    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "FolderCheckTimer::compareAllSqlState() throw db error:{}",
            e.what()
        );
    }
    return res;
}
void FolderCheckTimer::importNewFolder(const juce::File& folderPath) {
    try {
        mDbContext.dbManage.write([&](SQLite::Database& db) {
            SQLite::Transaction t{db};
            SQLite::Statement insertStmt{
                db,
                "INSERT INTO songFolders (dir,isRoot,lastModifiedTime) VALUES "
                "(:dir,1,:lastModifiedTime)"
            };
            insertStmt.bind(":dir", folderPath.getFullPathName().toStdString());
            insertStmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(folderPath));
            insertStmt.exec();

            auto childrenDir = folderPath.findChildFiles(juce::File::findDirectories, true);

            std::string insertChildDirSql =
                "INSERT OR IGNORE INTO songFolders (dir,isRoot,lastModifiedTime) VALUES ";
            for (int i = 0; i < childrenDir.size(); i++) {
                insertChildDirSql +=
                    "(:dir" + std::to_string(i) + ",0,:lastModifiedTime" + std::to_string(i) + ")";
                if (i != childrenDir.size() - 1) {
                    insertChildDirSql += ",";
                }
            }

            SQLite::Statement insertChildDirStmt{db, insertChildDirSql};
            for (int i = 0; i < childrenDir.size(); i++) {
                insertChildDirStmt.bind(
                    ":dir" + std::to_string(i),
                    childrenDir[i].getFullPathName().toStdString()
                );
                insertChildDirStmt.bind(
                    ":lastModifiedTime" + std::to_string(i),
                    FileUtils::getFileLastModifiedTime(childrenDir[i])
                );
            }
            insertChildDirStmt.exec();
            t.commit();

            auto childrenFile = folderPath.findChildFiles(juce::File::findFiles, true);
            for (auto& child : childrenFile) {
                auto checkRes = mSongImport.checkSongHasImport(child);
                if (checkRes == SongHasImportRes::HasImport) {
                    continue;
                }

                mSongImport.importSingleSong(checkRes, child, false);
            }
        });
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error(
            "SongFolderManage::insertFolderToTable throw db error:{}",
            e.what()
        );
    }
}