#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Transaction.h>
#include "fileMessage.hpp"

// ============================================================
// 建表 SQL
// ============================================================

// songs 表：存储文件层信息
inline const char* createSongsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS songs (
        song_id             INTEGER PRIMARY KEY AUTOINCREMENT,
        file_path           TEXT    UNIQUE NOT NULL,
        file_name           TEXT,
        file_size           INTEGER,
        last_modified_time  TEXT,
        add_time            TEXT,
        num_audio_streams   INTEGER,
        duration            REAL,
        title               TEXT,
        artist              TEXT,
        album               TEXT,
        album_artist        TEXT,
        genre               TEXT,
        track_number        INTEGER,
        disc_number         INTEGER,
        year                INTEGER,
        composer            TEXT,
        extra_metadata      TEXT,
        comment             TEXT,
        image_hash          TEXT
    )
)";

// streams 表：存储流层信息，通过 song_id 外键关联到 songs 表
inline const char* createStreamsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS streams (
        stream_id       INTEGER PRIMARY KEY AUTOINCREMENT,
        song_id         INTEGER NOT NULL,
        stream_count    INTEGER,
        bit_rate        INTEGER,
        sample_rate     INTEGER,
        num_channels    INTEGER,
        bit_depth       INTEGER,
        codec_name      TEXT,
        is_music        INTEGER,
        ai_genre        TEXT,
        ai_mood         TEXT,
        bpm             REAL,
        key             TEXT,
        ai_processed    INTEGER,
        extra_metadata  TEXT,
        FOREIGN KEY (song_id) REFERENCES songs(song_id) ON DELETE CASCADE
    )
)";

// 索引：加速按 song_id 查询 streams
inline const char* createStreamsIndexSQL = R"(
    CREATE INDEX IF NOT EXISTS idx_streams_song_id ON streams(song_id)
)";

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
