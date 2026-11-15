#include "./CenterDbManage.hpp"
#include "Utils/constants.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
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

/**
 * @brief 歌曲表
 * @param isSingleFile 这个歌曲是单独的文件还是随着某个文件夹导入的同时导入的文件
 * @param songFolderId 对应的这个音频文件放在哪个文件夹中，如果isSingleFile=true,则这个字段为空
 * @param filePath 这个歌曲「真正」的路径：
 *         · 文件夹导入的歌 = 用户磁盘上的原文件路径（文件夹视图靠它做
 *           「目录里的文件 → songId」的匹配，不能改成缓存路径）
 *         · 单独导入的歌 = <缓存目录>/songs/<原路径的sha1>.<扩展名>，也就是拷贝出来的
 *           那一份；原路径不入库（重复导入靠这个确定的缓存文件名判重）
 */
static const std::string createSongsTableSQL = std::format(
    R"(
        CREATE TABLE IF NOT EXISTS songs (
            songId             INTEGER PRIMARY KEY AUTOINCREMENT,
            filePath           TEXT    UNIQUE NOT NULL,
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
            lastPlayTime DATETIME,
            hash TEXT,
            albumId INTEGER,
            songFolderId INTEGER,
            isSingleFile    INTEGER DEFAULT 0,
            FOREIGN KEY (albumId) REFERENCES albums(albumId) ON DELETE CASCADE,
            FOREIGN KEY (songFolderId) REFERENCES songFolders(id) ON DELETE CASCADE
        )
    )",
    INT64T_MAX,
    INT64T_MAX,
    INT64T_MAX
);

static const char* createSongFolderIdIndexSQL =
    "CREATE INDEX IF NOT EXISTS songFolderIdIndex ON songs(songFolderId)";

static const char* createDurationIndexSQL =
    "CREATE INDEX IF NOT EXISTS durationIndex ON songs(duration)";

static const char* createPlayNumIndexSQL =
    "CREATE INDEX IF NOT EXISTS playNumIndex ON songs(playNum)";

// 最近播放排序要靠它，值的格式就是 SQLite 的 datetime('now','localtime')
static const char* createLastPlayTimeIndexSQL =
    "CREATE INDEX IF NOT EXISTS lastPlayTimeIndex ON songs(lastPlayTime)";

static const char* createAlbumDiscTrackIndexSQL =
    "CREATE INDEX IF NOT EXISTS albumDiscTrackIndex ON songs(albumId,discNumber,trackNumber)";

static const char* createAlbumIdIndexSQL =
    "CREATE INDEX IF NOT EXISTS albumIdIndex ON songs(albumId)";

/**
 * @brief 歌曲文件夹表
 * @param isRoot 这个文件夹是否为根目录
 * @param dir 文件夹绝对路径
 *
 * 注意：「单独导入」的歌**不会**进这张表。它们的音频文件会被拷到
 * <缓存目录>/songs 下，直接把缓存路径写进 songs.filePath。
 */
static const char* createSongFolderTableSQL = R"(
    CREATE TABLE IF NOT EXISTS songFolders (
        id     INTEGER PRIMARY KEY AUTOINCREMENT,
        dir TEXT NOT NULL UNIQUE,
        lastModifiedTime   TEXT NOT NULL ,
        isRoot INT NOT NULL
    )
)";

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
 * @param createTime description:添加到表中的时间
 */
static const char* createPlaylistTableSQL = R"(
    CREATE TABLE IF NOT EXISTS playlists(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        title TEXT NOT NULL,
        createTime TEXT NOT NULL DEFAULT (date('now', 'localtime')),
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

/**
 * @brief EQ 预设表
 * @param title 预设名（用户可改名）
 * @param params EQ 参数体 JSON，形状就是 EqWire::encodeAll 的输出（不含信封、不含 rev）。
 *               和"当前 EQ 状态"完全同构，所以读出来可以直接喂给 setEqAll 下发。
 * @param createTime 创建时间
 *
 * 本表只存用户显式保存过的预设；"当前 EQ 状态"由 EqController 持有，不入库。
 */
static const char* createEqPresetTableSQL = R"(
    CREATE TABLE IF NOT EXISTS eqPresets(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        title TEXT NOT NULL,
        createTime TEXT NOT NULL DEFAULT (date('now', 'localtime')),
        params TEXT NOT NULL
    )
)";

/**
 * @brief 歌词表
 *
 * 一首歌最多两条：kind = 'original' / 'translation'。
 * 存的是**原始文本**（LRC 全文），解析在读取时做，格式升级不用迁移数据。
 * sourcePath/mtime 记的是来源文件和它的修改时间，以后做"文件变了就重读"用。
 */
static const char* createLyricsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS lyrics(
        lyricId INTEGER PRIMARY KEY AUTOINCREMENT,
        songId INTEGER NOT NULL,
        kind TEXT NOT NULL,
        format TEXT NOT NULL,
        content TEXT NOT NULL,
        sourcePath TEXT,
        mtime INTEGER DEFAULT 0,
        UNIQUE(songId, kind),
        FOREIGN KEY (songId) REFERENCES songs(songId) ON DELETE CASCADE
    )
)";

static const char* createLyricsSongIdIndexSQL =
    "CREATE INDEX IF NOT EXISTS lyricsSongIdIndex ON lyrics(songId)";

/**
 * @brief 插件表（扫描结果）
 *
 * **主键是 (filePath, ordinal) 而不是 filePath**：VST3 通常一个模块一个插件，
 * 但 VST3 shell（`PluginDescription::hasSharedContainer`）一个 DLL 里会导出多个
 * 类型。用 filePath 单主键的话第二种会被 upsert 覆盖掉，而加一个"类型序号"
 * 就能把它们都存下来——顺便也让扫描失败的行（ordinal=0）有地方落。
 *
 * 几个字段的分工（规格 §4 + 建议补充）：
 *   type       ★用户可改★ 重扫**不覆盖**（upsert 里故意不写它）。
 *             元数据整条错掉的插件（OB-Xd 报 0/0）会落进 other，
 *             用户看得到、能改，这就是默认不走实例化扫描的兜底手段。
 *   name/vendor/category/numInputs/numOutputs
 *             元数据说了算，每次扫描刷新。
 *   identifier  PluginDescription::createIdentifierString()。
 *             将来做插件状态/预设时用它当键，换路径也不会丢；
 *             失败的扫描没有它，留空。
 *   scanStatus / scanError
 *             'ok' / 'failed'。没有这两列，扫描失败的插件在表里和正常插件
 *             长得一模一样，UI 根本没法提示"这个坏了"。
 *   lastSeen   这次扫描见到它的时间（毫秒）。以后做"这次没看到它 → 标灰"用。
 */
static const char* createPluginsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS plugins(
        filePath TEXT NOT NULL,
        ordinal INTEGER NOT NULL DEFAULT 0,
        identifier TEXT,
        name TEXT NOT NULL,
        vendor TEXT,
        category TEXT,
        type TEXT NOT NULL DEFAULT 'other',
        numInputs INTEGER DEFAULT 0,
        numOutputs INTEGER DEFAULT 0,
        scanStatus TEXT NOT NULL DEFAULT 'ok',
        scanError TEXT,
        lastSeen INTEGER DEFAULT 0,
        PRIMARY KEY (filePath, ordinal)
    )
)";

static const char* createPluginIdentifierIndexSQL =
    "CREATE INDEX IF NOT EXISTS pluginIdentifierIndex ON plugins(identifier)";

/**
 * @brief 插件扫描目录
 *
 * @param isBuiltin 系统默认目录（`C:\Program Files\Common Files\VST3` 之类）。
 *                  默认目录在启动时无条件写入，且**不允许删除**——用户只能
 *                  增删自己的目录。目录被移除后 plugins 里的行**保留**
 *                  （不级联删除），UI 汇总时按目录过滤即可。
 */
static const char* createPluginDirsTableSQL = R"(
    CREATE TABLE IF NOT EXISTS pluginDirs(
        path TEXT PRIMARY KEY,
        isBuiltin INTEGER NOT NULL DEFAULT 0
    )
)";

CenterDbManage::CenterDbManage(const std::string& path)    : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {

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
        db.exec(createSongFolderTableSQL);
        db.exec(createEqPresetTableSQL);
        db.exec(createLyricsTableSQL);
        db.exec(createPluginsTableSQL);
        db.exec(createPluginDirsTableSQL);

        db.exec(createPlaylistsSongsOrderByUserIndex);
        db.exec(createAlbumDiscTrackIndexSQL);
        db.exec(createAlbumIdIndexSQL);
        db.exec(createDurationIndexSQL);
        db.exec(createPlayNumIndexSQL);
        db.exec(createLastPlayTimeIndexSQL);
        db.exec(createSongFolderIdIndexSQL);
        db.exec(createLyricsSongIdIndexSQL);
        db.exec(createPluginIdentifierIndexSQL);

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
