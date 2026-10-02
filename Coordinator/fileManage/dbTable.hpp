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
        albumArtist        TEXT,
        genre              TEXT,
        trackNumber        INTEGER,
        discNumber         INTEGER,
        year               INTEGER,
        composer           TEXT,
        bitRate            INTEGER,
        bitDepth           INTEGER,
        sampleRate         INTEGER,
        channelLayoutMask INTEGER,
        numChannels INYEGER,
        codecName TEXT, 
        aiGenre            TEXT,
        bpm                INTEGER,
        key                TEXT,
        aiProcessed        INTEGER DEFAULT 0,
        isMyLike           INTEGER DEFAULT 0,
        comment            TEXT,
        playNum       INTEGER DEFAULT 0,
        hash TEXT,
        timeDomainSpec BLOB
    )
)";