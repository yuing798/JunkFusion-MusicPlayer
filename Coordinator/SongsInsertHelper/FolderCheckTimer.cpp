#include "./FolderCheckTimer.hpp"
#include "Utils/constants.h"
#include "Utils/fileUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

FolderCheckTimer::FolderCheckTimer(DbContext& dbContext, SongImport& songImport)
    : juce::Thread("FolderCheckTimer"), mDbContext(dbContext), mSongImport(songImport) {}

FolderCheckTimer::~FolderCheckTimer() { stop(); }

// ════════════════════════════════════════════════════════════════
// 线程
// ════════════════════════════════════════════════════════════════

void FolderCheckTimer::start() {
    if (isThreadRunning()) return;
    startThread(juce::Thread::Priority::normal);
}

void FolderCheckTimer::stop() {
    if (!isThreadRunning()) return;

    signalThreadShouldExit();
    notify();

    // -1 = 一直等到线程真的退出：它还在用它引用的 SongImport / DbContext，
    // 必须在这两个析构之前 join 掉
    stopThread(-1);
}

void FolderCheckTimer::run() {
    while (!threadShouldExit()) {
        // wait 返回 true 说明被 notify 了（一般就是 stop）
        if (wait(static_cast<double>(checkIntervalMs))) break;
        if (threadShouldExit()) break;

        checkOnce();
    }
}

// ════════════════════════════════════════════════════════════════
// 一轮检查
// ════════════════════════════════════════════════════════════════

void FolderCheckTimer::checkOnce() {
    // 上一轮还没跑完（比如正在导入一个几百首的目录）就直接跳过这一轮，
    // 不然任务会一轮轮叠上来
    const juce::ScopedTryLock lock(mCheckLock);
    if (!lock.isLocked()) {
        if (auto logger = spdlog::get(LogDllID))
            logger->debug("上一轮文件夹检查还没结束，跳过这一轮");
        return;
    }

    const auto changes = compareAllDirState();
    if (changes.empty()) return;

    const auto dirsToSync = applyStructureChanges(changes);
    for (const auto& dir : dirsToSync) {
        if (threadShouldExit()) break;
        syncDirContents(dir);
    }
}

// ════════════════════════════════════════════════════════════════
// ① 结构比对（只读）
// ════════════════════════════════════════════════════════════════

std::unordered_map<std::string, FolderCheckTimer::DirState> FolderCheckTimer::compareAllDirState() {
    std::unordered_map<std::string, DirState> result;

    struct TableRow {
        std::string dir;
        std::string lastModifiedTime;
        bool isRoot{false};
    };
    std::vector<TableRow> rows;

    // 表里的行一次读出来，后面全在内存里比
    try {
        SQLite::Statement stmt{
            mDbContext.dbManage.readDb(),
            "SELECT dir,lastModifiedTime,isRoot FROM songFolders"
        };
        while (stmt.executeStep()) {
            rows.push_back(
                {stmt.getColumn("dir").getString(),
                 stmt.getColumn("lastModifiedTime").getString(),
                 stmt.getColumn("isRoot").getInt() != 0}
            );
        }
    } catch (SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("FolderCheckTimer::compareAllDirState 读 songFolders 失败:{}", e.what());
        return result;
    }

    std::unordered_set<std::string> dirsInTable;
    dirsInTable.reserve(rows.size());
    for (const auto& row : rows)
        dirsInTable.insert(row.dir);

    // 表里的目录：还在不在、时间戳变没变
    for (const auto& row : rows) {
        const juce::File dir{row.dir};

        if (!dir.isDirectory()) {
            result[row.dir] = {SqlState::Delete, row.isRoot};
            continue;
        }

        if (FileUtils::getFileLastModifiedTime(dir) != row.lastModifiedTime) {
            result[row.dir] = {SqlState::Update, row.isRoot};
        }
    }

    // 真实目录里表里没有的算新增：只从还活着的根目录往下找
    for (const auto& row : rows) {
        if (!row.isRoot) continue;

        const juce::File rootDir{row.dir};
        if (!rootDir.isDirectory()) continue;

        for (const auto& child : rootDir.findChildFiles(juce::File::findDirectories, true)) {
            const std::string childPath = child.getFullPathName().toStdString();
            if (dirsInTable.find(childPath) != dirsInTable.end()) continue;

            result[childPath] = {SqlState::Add, false};
        }
    }

    return result;
}

// ════════════════════════════════════════════════════════════════
// ② 结构变化落库（一个事务，只有 SQL）
// ════════════════════════════════════════════════════════════════

std::vector<std::string>
FolderCheckTimer::applyStructureChanges(const std::unordered_map<std::string, DirState>& changes) {
    std::vector<std::string> dirsToSync; // 之后还要做文件级比对的目录

    std::vector<std::string> dirsToInsert;
    std::vector<std::string> dirsToRefresh;
    std::vector<std::pair<int64_t, std::string>> dirsToDelete; // id, dir

    // 被删掉的根目录：它下面的子目录行也要一起删（songFolders 没有父子关系，
    // 不删就是孤儿行；songs 会靠外键级联删掉）
    std::vector<std::string> deletedRootDirs;

    for (const auto& [dir, state] : changes) {
        if (state.sqlState == SqlState::Add) dirsToInsert.push_back(dir);
        if (state.sqlState == SqlState::Delete && state.isRoot) deletedRootDirs.push_back(dir);
    }

    struct Row {
        int64_t id{0};
        std::string dir;
    };
    std::vector<Row> rowsFromTable;
    try {
        SQLite::Statement stmt{mDbContext.dbManage.readDb(), "SELECT id,dir FROM songFolders"};
        while (stmt.executeStep()) {
            rowsFromTable.push_back(
                {stmt.getColumn("id").getInt64(), stmt.getColumn("dir").getString()}
            );
        }
    } catch (SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error(
                "FolderCheckTimer::applyStructureChanges 读 songFolders 失败:{}",
                e.what()
            );
        return dirsToSync;
    }

    for (const auto& row : rowsFromTable) {
        const auto it = changes.find(row.dir);

        bool needDelete = it != changes.end() && it->second.sqlState == SqlState::Delete;

        if (!needDelete) {
            for (const auto& rootDir : deletedRootDirs) {
                if (isPathUnderDir(row.dir, rootDir)) {
                    needDelete = true;
                    break;
                }
            }
        }

        if (needDelete) {
            dirsToDelete.push_back({row.id, row.dir});
            continue;
        }

        if (it != changes.end() && it->second.sqlState == SqlState::Update) {
            dirsToRefresh.push_back(row.dir);
        }
    }

    // 删之前先把这些目录下的 songId 收集起来，删完通知前端
    std::vector<int64_t> deletedSongIds;
    if (!dirsToDelete.empty()) {
        try {
            for (const auto& [id, dir] : dirsToDelete) {
                SQLite::Statement stmt{
                    mDbContext.dbManage.readDb(),
                    "SELECT songId FROM songs WHERE songFolderId=:songFolderId"
                };
                stmt.bind(":songFolderId", id);
                while (stmt.executeStep()) {
                    deletedSongIds.push_back(stmt.getColumn("songId").getInt64());
                }
            }
        } catch (SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error(
                    "FolderCheckTimer::applyStructureChanges 读待删歌曲失败:{}",
                    e.what()
                );
        }
    }

    try {
        mDbContext.dbManage.write([&](SQLite::Database& db) {
            SQLite::Transaction transaction{db};

            for (const auto& [id, dir] : dirsToDelete) {
                SQLite::Statement stmt{db, "DELETE FROM songFolders WHERE id=:id"};
                stmt.bind(":id", id);
                stmt.exec();
            }

            for (const auto& dir : dirsToInsert) {
                SQLite::Statement stmt{
                    db,
                    "INSERT OR IGNORE INTO songFolders (dir,isRoot,lastModifiedTime) "
                    "VALUES (:dir,0,:lastModifiedTime)"
                };
                stmt.bind(":dir", dir);
                stmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(juce::File{dir}));
                stmt.exec();
            }

            for (const auto& dir : dirsToRefresh) {
                SQLite::Statement stmt{
                    db,
                    "UPDATE songFolders SET lastModifiedTime=:lastModifiedTime WHERE dir=:dir"
                };
                stmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(juce::File{dir}));
                stmt.bind(":dir", dir);
                stmt.exec();
            }

            transaction.commit();
        });
    } catch (const std::exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("FolderCheckTimer::applyStructureChanges 写库失败:{}", e.what());
        return dirsToSync;
    }

    for (const auto songId : deletedSongIds) {
        if (mSongImport.onDeleteSong) mSongImport.onDeleteSong(songId);
    }

    // 新增的目录要补歌；时间戳变了的目录要重新比对文件
    dirsToSync.insert(dirsToSync.end(), dirsToInsert.begin(), dirsToInsert.end());
    dirsToSync.insert(dirsToSync.end(), dirsToRefresh.begin(), dirsToRefresh.end());

    return dirsToSync;
}

// ════════════════════════════════════════════════════════════════
// ③ 文件级比对（事务外）
// ════════════════════════════════════════════════════════════════

void FolderCheckTimer::syncDirContents(const std::string& dirPath) {
    const juce::File dir{dirPath};
    if (!dir.isDirectory()) return;

    int64_t folderId{0};
    try {
        SQLite::Statement stmt{
            mDbContext.dbManage.readDb(),
            "SELECT id FROM songFolders WHERE dir=:dir"
        };
        stmt.bind(":dir", dirPath);
        if (!stmt.executeStep()) return;
        folderId = stmt.getColumn("id").getInt64();
    } catch (SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("FolderCheckTimer::syncDirContents 取目录 id 失败:{}", e.what());
        return;
    }

    // 库里这个目录下的歌：filePath -> songId
    std::unordered_map<std::string, int64_t> songsInTable;
    try {
        SQLite::Statement stmt{
            mDbContext.dbManage.readDb(),
            "SELECT songId,filePath FROM songs WHERE songFolderId=:songFolderId"
        };
        stmt.bind(":songFolderId", folderId);
        while (stmt.executeStep()) {
            songsInTable[stmt.getColumn("filePath").getString()] =
                stmt.getColumn("songId").getInt64();
        }
    } catch (SQLite::Exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("FolderCheckTimer::syncDirContents 读目录歌曲失败:{}", e.what());
        return;
    }

    // 盘上的文件：表里没有的、或者文件 mtime 变了的都要导一遍
    std::unordered_set<std::string> filesOnDisk;
    for (const auto& file : dir.findChildFiles(juce::File::findFiles, false)) {
        if (threadShouldExit()) return;

        const std::string filePath = file.getFullPathName().toStdString();
        filesOnDisk.insert(filePath);

        const auto checkRes = mSongImport.checkSongHasImport(file, false);
        if (checkRes == SongHasImportRes::HasImport) continue;

        if (!mSongImport.importSingleSong(checkRes, file, false)) {
            if (auto logger = spdlog::get(LogDllID)) logger->warn("定时检查导入失败:{}", filePath);
        }
    }

    // 表里有、盘上没有的：删行并通知前端（文件本来就不在了，不用动磁盘）
    for (const auto& [filePath, songId] : songsInTable) {
        if (filesOnDisk.find(filePath) != filesOnDisk.end()) continue;

        try {
            mDbContext.dbManage.write([songId](SQLite::Database& db) {
                SQLite::Statement stmt{db, "DELETE FROM songs WHERE songId=:songId"};
                stmt.bind(":songId", songId);
                stmt.exec();
            });
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("FolderCheckTimer::syncDirContents 删歌失败:{}", e.what());
            continue;
        }

        if (mSongImport.onDeleteSong) mSongImport.onDeleteSong(songId);
    }
}

// ════════════════════════════════════════════════════════════════
// 前端：导入整个文件夹
// ════════════════════════════════════════════════════════════════

void FolderCheckTimer::importNewFolder(const juce::File& folderPath) {
    if (!folderPath.isDirectory()) {
        if (auto logger = spdlog::get(LogDllID))
            logger->warn(
                "导入文件夹失败，不是有效目录:{}",
                folderPath.getFullPathName().toStdString()
            );
        return;
    }

    // 别和定时检查撞车（反过来说，导入期间定时检查会自己跳过这一轮）
    const juce::ScopedLock lock(mCheckLock);

    if (mDbContext.songFolderManage.isDirHasImport(folderPath)) {
        if (auto logger = spdlog::get(LogDllID))
            logger->warn("文件夹已经导入过:{}", folderPath.getFullPathName().toStdString());
        return;
    }

    // ① 目录行：一个事务，只有 SQL
    const auto childDirs = folderPath.findChildFiles(juce::File::findDirectories, true);
    try {
        mDbContext.dbManage.write([&](SQLite::Database& db) {
            SQLite::Transaction transaction{db};

            {
                SQLite::Statement stmt{
                    db,
                    "INSERT OR IGNORE INTO songFolders (dir,isRoot,lastModifiedTime) "
                    "VALUES (:dir,1,:lastModifiedTime)"
                };
                stmt.bind(":dir", folderPath.getFullPathName().toStdString());
                stmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(folderPath));
                stmt.exec();
            }

            for (const auto& child : childDirs) {
                SQLite::Statement stmt{
                    db,
                    "INSERT OR IGNORE INTO songFolders (dir,isRoot,lastModifiedTime) "
                    "VALUES (:dir,0,:lastModifiedTime)"
                };
                stmt.bind(":dir", child.getFullPathName().toStdString());
                stmt.bind(":lastModifiedTime", FileUtils::getFileLastModifiedTime(child));
                stmt.exec();
            }

            transaction.commit();
        });
    } catch (const std::exception& e) {
        if (auto logger = spdlog::get(LogDllID))
            logger->error("FolderCheckTimer::importNewFolder 写目录失败:{}", e.what());
        return;
    }

    // ② 文件导入：事务外，每首歌自己走一次 write
    for (const auto& file : folderPath.findChildFiles(juce::File::findFiles, true)) {
        if (threadShouldExit()) return;

        const auto checkRes = mSongImport.checkSongHasImport(file, false);
        if (checkRes == SongHasImportRes::HasImport) continue;

        if (!mSongImport.importSingleSong(checkRes, file, false)) {
            if (auto logger = spdlog::get(LogDllID))
                logger->warn("导入文件夹时跳过:{}", file.getFullPathName().toStdString());
        }
    }
}

// ════════════════════════════════════════════════════════════════
// 小工具
// ════════════════════════════════════════════════════════════════

std::string FolderCheckTimer::withTrailingSeparator(const std::string& dir) {
    if (dir.empty()) return dir;

    const char last = dir.back();
    if (last == '\\' || last == '/') return dir;

    const char sep = static_cast<char>(juce::File::getSeparatorChar());
    return dir + sep;
}
bool FolderCheckTimer::isPathUnderDir(const std::string& path, const std::string& dir) {
    if (path == dir) return true;

    const std::string prefix = withTrailingSeparator(dir);
    return path.rfind(prefix, 0) == 0;
}
