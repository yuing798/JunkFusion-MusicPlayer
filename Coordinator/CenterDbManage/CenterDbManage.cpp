#include "./CenterDbManage.hpp"
#include "Utils/constants.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstring>
#include <format>
#include <spdlog/spdlog.h>
#include <string>

// ============================================================================
// 曲库表结构（SQLite）
//
//    albums ─┐
//            ├─< songs >───< playlistSongs >─── playlists
//            │
//
// 为什么歌单要单独一张关联表：
//   * 专辑和歌曲是「一对多」（一首歌只属于一张专辑），所以 albums 的主键直接
//     放在 songs 里（songs.albumId）就够了；
//   * 歌单和歌曲是「多对多」：同一首歌可以同时待在「通勤」「学习」「电子」好几个
//     歌单里，所以两边都不能存对方的 id：
//       - 在 songs 里放 playlistId → 一首歌只能属于一个歌单（以前就是这么写的，废弃了）；
//       - 在 playlists 里存 songId 的 JSON 字符串 → 没法建索引、没法按顺序取歌，
//         删一首歌得把整行读出来改完再写回去。
//     最后用中间表 playlistSongs 表达关联，一行 = 「某个歌单里有某首歌」，
//     加歌/删歌/排序都只动这一张表，songs 里的歌曲数据一行都不用碰。
//
// 所有建表语句都是 IF NOT EXISTS，启动时无条件跑一遍即可（旧库不会被破坏）。
// ============================================================================

static const std::string createSongsTableSQL = std::format(
    R"(
        CREATE TABLE IF NOT EXISTS songs (
            songId             INTEGER PRIMARY KEY AUTOINCREMENT,
            filePath           TEXT    UNIQUE NOT NULL,
            fileSize           INTEGER NOT NULL,
            lastModifiedTime   TEXT    NOT NULL,
            duration           REAL NOT NULL,
            title              TEXT NOT NULL,
            artists             TEXT,
            album              TEXT,
            trackNumber        INTEGER DEFAULT {},
            discNumber INTEGER DEFAULT {},
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
            FOREIGN KEY (albumId) REFERENCES albums(albumId) ON DELETE CASCADE
        )
    )",
    INT64T_MAX,
    INT64T_MAX,
    INT64T_MAX
);

static const char* createDurationIndexSQL =
    "CREATE INDEX IF NOT EXISTS durationIndex ON songs(duration)";

static const char* createPlayNumIndexSQL =
    "CREATE INDEX IF NOT EXISTS playNumIndex ON songs(playNum)";

static const char* createAlbumDiscTrackIndexSQL =
    "CREATE INDEX IF NOT EXISTS albumDiscTrackIndex ON songs(albumId,discNumber,trackNumber)";

static const char* createAlbumIdIndexSQL =
    "CREATE INDEX IF NOT EXISTS albumIdIndex ON songs(albumId)";

/**
 * @brief 专辑表
 * @param albumArtists 获取到的专辑艺术家名字
 * @param artists 如果没有专辑艺术家，获取到的歌曲艺术家名字
 * @param displayArtists 有专辑艺术家即展示专辑艺术家，没有就展示艺术家
 */
static const char* createAlbumsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS albums (
        albumId             INTEGER PRIMARY KEY AUTOINCREMENT,
        title              TEXT,
        albumArtists             TEXT,
        hash TEXT
    )
)";

/**
 * @brief 歌单表
 * @param id 歌单 id（前端拿到的是 playlistId，就是这一列）
 * @param title 歌单名
 * @param hash 歌单封面的图片哈希，指向 <缓存目录>/image/<hash>/original.jpg
 *             （和歌曲封面共用同一套按哈希去重的图片缓存）。
 *             为 NULL 表示没有单独设置封面，视图层会自动退回
 *             「歌单里第一首有封面的歌」（见 PlayListManage.cpp 里的视图查询）
 * @param createdAt description:添加到表中的时间
 */
static const char* createPlaylistTableSQL = R"(
    CREATE TABLE IF NOT EXISTS playlists(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        title TEXT NOT NULL,
        createdAt TEXT NOT NULL DEFAULT (date('now', 'localtime')),
        hash TEXT
    )
)";

/**
 * @brief 歌单-歌曲关联表（多对多）
 * @param playlistId 歌单 id，指向 playlists.id
 * @param songId 歌曲 id，指向 songs.songId
 *
 * 复合主键 (playlistId, songId) 有两个作用：
 *   1. 保证同一首歌在同一个歌单里只出现一次（重复添加用 INSERT OR IGNORE 静默跳过）；
 *   2. 它本身就是一个 (playlistId, songId) 索引，「取某个歌单里的歌」很快。
 *
 * 两个外键都带 ON DELETE CASCADE：删歌单时关联行自动清掉，删歌曲时也不会留下
 * 指向不存在歌曲的孤儿行。（删歌单的代码里仍然会显式删一遍关联，
 * 因为旧库的外键开关不一定打开过。）
 */
static const std::string createPlaylistSongsTableSQL = std::format(
    R"(
        CREATE TABLE IF NOT EXISTS playlistSongs(
            playlistId INTEGER NOT NULL,
            songId INTEGER NOT NULL,
            orderByUser INTEGER DEFAULT {},
            PRIMARY KEY (playlistId, songId),
            FOREIGN KEY (playlistId) REFERENCES playlists(id) ON DELETE CASCADE,
            FOREIGN KEY (songId) REFERENCES songs(songId) ON DELETE CASCADE
        )
    )",
    INT64T_MAX
);

static const char* createPlaylistsSongsOrderByUserIndex =
    "CREATE INDEX IF NOT EXISTS orderByUserIndex ON playlistSongs(playlistId,orderByUser,songId)";

CenterDbManage::CenterDbManage(const std::string& path)
    : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {

    try {

        db.exec("PRAGMA busy_timeout = 5000;");
        db.exec("PRAGMA foreign_keys = ON;");
        db.exec("PRAGMA synchronous = NORMAL;");

        db.exec("PRAGMA journal_mode = WAL;");

        // 建表顺序按依赖关系来：被引用的表在前，引用别人的表在后
        db.exec(createAlbumsTableSQL);
        db.exec(createSongsTableSQL);
        db.exec(createPlaylistTableSQL);
        db.exec(createPlaylistSongsTableSQL);

        db.exec(createPlaylistsSongsOrderByUserIndex);
        db.exec(createAlbumDiscTrackIndexSQL);
        db.exec(createAlbumIdIndexSQL);
        db.exec(createDurationIndexSQL);
        db.exec(createPlayNumIndexSQL);

        if (auto logger{spdlog::get(LogDllID)}) {
            logger->debug("数据库初始化完成:{}", path);
        }

    } catch (const std::exception& e) {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogDllID);
        if (logger)
            logger->critical(
                "无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}",
                e.what()
            );
    }
}
