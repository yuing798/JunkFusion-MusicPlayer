#pragma once

// songs 表：存储所有歌曲信息（文件层信息 + FFmpeg 解码层信息 + AI 分析信息 + 用户信息）
inline const char* createSongsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS songs (
        songId             INTEGER PRIMARY KEY AUTOINCREMENT,
        filePath           TEXT    UNIQUE NOT NULL,
        fileSize           INTEGER NOT NULL,
        lastModifiedTime   TEXT    NOT NULL,
        duration           REAL,
        title              TEXT,
        artists             TEXT,
        album              TEXT,
        trackNumber        INTEGER,
        discNumber INTEGER
        bitRate            INTEGER,
        bitDepth           INTEGER,
        sampleRate         INTEGER,
        channelLayoutMask INTEGER,
        codecName TEXT, 
        bpm                INTEGER,
        key                TEXT,
        isMyLike           INTEGER DEFAULT 0,
        playNum       INTEGER DEFAULT 0,
        hash TEXT,
        timeDomainSpec BLOB,
        albumId INTEGER,
        FOREIGN KEY (albumId) REFERENCES albums(albumId)
    )
)";
/**
 * @brief 专辑表
 * @param albumArtists 获取到的专辑艺术家名字
 * @param artists 如果没有专辑艺术家，获取到的歌曲艺术家名字
 * @param displayArtists 有专辑艺术家即展示专辑艺术家，没有就展示艺术家
 */
inline const char* createAlbumsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS albums (
        albumId             INTEGER PRIMARY KEY AUTOINCREMENT,
        title              TEXT,
        albumArtists             TEXT,
        hash TEXT
    )
)";