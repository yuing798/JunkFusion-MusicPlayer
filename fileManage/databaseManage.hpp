#pragma once

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Transaction.h>
#include <memory>
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include <spdlog/spdlog.h>

// ============================================================
// 建表 SQL
// ============================================================

// songs 表：存储所有歌曲信息（文件层信息 + FFmpeg 解码层信息 + AI 分析信息 + 用户信息）
//PRIMARY KEY:主键 AUTOINCREMENT：自动递增
//NOT NULL:禁止留空
//UNIQUE:唯一
//REAL:存储浮点数
inline const char* createSongsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS songs (
        songId             INTEGER PRIMARY KEY AUTOINCREMENT,
        filePath           TEXT    UNIQUE NOT NULL,
        fileName           TEXT    NOT NULL,
        fileSize           INTEGER NOT NULL,
        lastModifiedTime   TEXT    NOT NULL,
        addTime            TEXT,
        isMultiStreamFile  INTEGER,
        duration           REAL,
        title              TEXT,
        artist             TEXT,
        album              TEXT,
        albumArtist        TEXT,
        genre              TEXT,
        trackNumber        INTEGER,
        discNumber         INTEGER,
        year               INTEGER,
        composer           TEXT,
        imageHash          TEXT,
        bitRate            INTEGER,
        bitDepth           INTEGER,
        sampleRate         INTEGER,
        numChannels        INTEGER,
        codecName          TEXT,
        isMusic            INTEGER DEFAULT 0,
        aiGenre            TEXT,
        aiMood             TEXT,
        bpm                INTEGER DEFAULT 0,
        key                TEXT,
        aiProcessed        INTEGER DEFAULT 0,
        isMyLike           INTEGER DEFAULT 0,
        comment            TEXT,
        hadPlayedNum       INTEGER DEFAULT 0
    )
)";



class SongsManage{
private:
    std::unique_ptr<SQLite::Database> db;
    juce::File songsDbFile;//歌曲管理文件

    /** 创建 songs 表，如果已存在则跳过 */
    void createTables();
    
public:

    /** 检查 filePath 是否已在数据库中（仅按路径匹配，不做 size/time 比较） */
    bool isSongExists(const std::string& filePath);
    SongsManage();
    ~SongsManage();

    /**
     * 插入一首歌，带事务保护。
     *
     * 防重复策略：
     *   1. 数据库层：filePath 有 UNIQUE 约束
     *   2. 代码层：插入前先查询 filePath，比较 fileSize 和 lastModifiedTime
     *      - 完全相同 → 视为重复，跳过插入
     *      - 不同 → 文件已更新，更新旧记录
     *      - 不存在 → 正常插入
     *
     * @param info     歌曲信息
     */
    void insertSong(const SongInfo& info);

    /** 获取歌曲总数 */
    int getTotalSongCount();

    /** 分页获取歌曲，offset 从 0 开始，返回 limit 条记录 */
    std::vector<SongInfo> getSongsPage(int offset, int limit);

    static SongsManage& getInstance() {
        static SongsManage instance; //首次调用时创建，程序结束时自动析构
        return instance;
    }

    SongsManage(const SongsManage&) = delete;
    SongsManage& operator=(const SongsManage&) = delete;
    SongsManage(SongsManage&&) = delete;
    SongsManage& operator=(SongsManage&&) = delete;//强调全局唯一单例
};
