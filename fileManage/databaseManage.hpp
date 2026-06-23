#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Transaction.h>
#include "fileMessage.hpp"

// ============================================================
// 建表 SQL
// ============================================================

// songs 表：存储文件层信息
//PRIMARY KEY:主键 AUTOINCREMENT：自动递增
//NOT NULL:禁止留空
//UNIQUE:唯一
//REAL:存储浮点数
inline const char* createSongsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS songs (
        songId             INTEGER PRIMARY KEY AUTOINCREMENT,
        filePath           TEXT    UNIQUE NOT NULL,
        fileName           TEXT NOT NULL,
        fileSize           INTEGER NOT NULL,
        lastModifiedTime  TEXT NOT NULL,
        addTime            TEXT,
        numAudioStreams   INTEGER,
        duration            REAL,
        title               TEXT,
        artist              TEXT,
        album               TEXT,
        albumArtist        TEXT,
        genre               TEXT,
        trackNumber        INTEGER,
        discNumber         INTEGER,
        year                INTEGER,
        composer            TEXT,
        extraMetadata      TEXT,
        comment             TEXT,
        imageHash          TEXT
    )
)";

// streams 表：存储流层信息，通过 song_id 外键关联到 songs 表
inline const char* createStreamsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS streams (
        streamId       INTEGER PRIMARY KEY AUTOINCREMENT,
        songId         INTEGER NOT NULL,
        streamCount    INTEGER,
        bitRate        INTEGER,
        sampleRate     INTEGER,
        numChannels    INTEGER,
        bitDepth       INTEGER,
        codecName      TEXT,
        isMusic        INTEGER,
        aiGenre        TEXT,
        aiGood         TEXT,
        bpm             INTEGER,
        key             TEXT,
        aiProcessed    INTEGER,
        extra_metadata  TEXT,
        FOREIGN KEY (songId) REFERENCES songs(songId) ON DELETE CASCADE
    )
)";
//FOREIGN KEY (song_id) REFERENCES songs(song_id) ON DELETE CASCADE
//FOREIGN KEY (song_id):这是从外界借来的键
//REFERENCES songs(song_id):参考外界的songs表中的song_id
//ON DELETE CASCADE:母表的song_id删除，所有相对应的流表也要删除

// 索引：加速按 song_id 查询 streams
inline const char* createStreamsIndexSQL = R"(
    CREATE INDEX IF NOT EXISTS idx_streams_songId ON streams(songId)
)";
//CREATE INDEX：创建一个索引（可以理解为给数据库做一张“目录”或“快捷方式”）
//IF NOT EXISTS：如果这个索引不存在才创建，防止重复执行 SQL 时报错
//idx_streams_song_id：给这个索引起的名字（习惯用 idx_表名_字段名 的格式）
//ON streams(song_id)：在 streams 表的 song_id 这一列上建立索引
//如果你没有提前创建索引，数据库就只能把整个表从头到尾翻一遍（全表扫描）来找匹配的行

// ============================================================
// API 函数声明
// ============================================================

/** 创建所有表（songs + streams）和索引，如果已存在则跳过 */
void createTables(SQLite::Database& db);

/**
 * 插入一首歌及其所有流，带事务保护。
 *
 * 防重复策略：
 *   1. 数据库层：file_path 有 UNIQUE 约束
 *   2. 代码层：插入前先查询 file_path，比较 file_size 和 last_modified_time
 *      - 完全相同 → 视为重复，跳过插入，返回已有 song_id
 *      - 不同 → 文件已更新，删除旧记录后重新插入
 *      - 不存在 → 正常插入
 *
 * @param db       SQLite 数据库连接
 * @param info     歌曲信息（包含文件层和流层数据）
 * @return         song_id（成功），-1（失败）
 */
int insertSong(SQLite::Database& db, const SongInfo& info);

/** 检查 filePath 是否已在数据库中（仅按路径匹配，不做 size/time 比较） */
bool isSongExists(SQLite::Database& db, const std::string& filePath);
