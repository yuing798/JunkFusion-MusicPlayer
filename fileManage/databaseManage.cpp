#include "databaseManage.hpp"
// #include "FontAbout/font.h"
#include "constants.h"
#include "fileMessage.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <unicode/coll.h>
#include <unicode/locid.h>
#include <unicode/stringpiece.h>
#include <vector>

SongsManage::SongsManage()
:db(nullptr){
    
}
void SongsManage::init(){
    // 使用 constants.h 中统一定义的 databaseDirId，避免路径不一致
    if (!databaseDirId.exists())
        databaseDirId.createDirectory();

    songsDbFile = databaseDirId.getChildFile("songs.db");

    try
    {
        db = std::make_unique<SQLite::Database>(
            songsDbFile.getFullPathName().toStdString(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        createTables();
    }
    catch (const std::exception& e)
    {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogAllID);
        if (logger) logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}", e.what());
    }
}

// ============================================================
// createTables
// ============================================================
void SongsManage::createTables()
{
    db->exec(createSongsTableSQL);
    db->exec(createNameIdIndexSQL);
    //db.exec() 这个函数的全称是 “执行 SQL 语句”，而不是”创建表”
    /*
    SQL命令：
    CREATE TABLE ...	在硬盘里划分一块区域，建一栋楼（表）存放数据。
    CREATE INDEX ...	在硬盘里划分另一块区域，建一部直达电梯（索引）。
    INSERT INTO ...	往楼里搬家具（插入数据行）。
    DROP TABLE ...	把整栋楼爆破拆除（删除表）
    */
}

// ============================================================
// isSongExists
// ============================================================
bool SongsManage::isSongExists(const std::string& filePath)
{
    if (!db) return false;
    //SQLite::Database:存储数据库连接句柄（一个指向 .db 文件的指针）、连接状态、是否开启事务等管理信息。它是你操作数据库的“总入口”。
    //SQLite::Statement存储预编译好的 SQL 语句模板（比如 SELECT * FROM songs WHERE id = ?）、
    // 绑定的参数值（你填入的 filePath）、以及当前正在读取的那一行数据（执行查询后的结果缓冲区）。
    SQLite::Statement query(*db, "SELECT COUNT(*) FROM songs WHERE filePath = :filePath");
    //告诉数据库”我要数一下，songs 表里有多少行的 filePath 等于后面那个参数”。
    //:filePath 是一个”命名占位符”，专门留给后面的 C++ 变量来填的
    //目的：防止 SQL 注入攻击，同时命名参数比 ? 可读性更好
    //SQLite 会先把带占位符的 SQL 编译成”执行计划”，然后只替换占位符的值。
    //如果你要循环插入一万首歌，这种写法比拼字符串快得多。
    query.bind(":filePath", filePath);
    //把函数传进来的 filePath（比如 “C:\Music\Adele.mp3”）塞到刚才那个 :filePath 的位置上
    //命名参数绑定看占位符的名字（含冒号前缀），列读取看”SELECT 写的顺序”
    query.executeStep();
    //执行查询：数据库跑去找数据。
    return query.getColumn(0).getInt() > 0;
    //COUNT(*) 没有列名，所以仍然用数字索引（从 0 开始）
}

// ============================================================
// insertSong
// ============================================================
void SongsManage::insertSong(const SongInfo& info)
{
    if (!db) return;
    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&info](SQLite::Statement& stmt) {
        stmt.bind(":filePath",         info.filePath);
        stmt.bind(":fileSize",         static_cast<int64_t>(info.fileSize));
        stmt.bind(":lastModifiedTime", info.lastModifiedTime);
        stmt.bind(":isMultiStreamFile",info.isMultiStreamFile);
        stmt.bind(":duration",         info.duration);
        stmt.bind(":title",            info.title);
        stmt.bind(":artist",           info.artist);
        stmt.bind(":album",            info.album);
        stmt.bind(":albumArtist",      info.albumArtist);
        stmt.bind(":genre",            info.genre);
        stmt.bind(":trackNumber",      info.trackNumber);
        stmt.bind(":discNumber",       info.discNumber);
        stmt.bind(":year",             info.year);
        stmt.bind(":composer",         info.composer);
        stmt.bind(":imageHash",        info.imageHash);
        stmt.bind(":bitRate",          info.bitRate);
        stmt.bind(":bitDepth",         info.bitDepth);
        stmt.bind(":sampleRate",       info.sampleRate);
        stmt.bind(":numChannels",      info.numChannels);
        stmt.bind(":codecName",        info.codecName);
    };

    try
    {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(*db,
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = :filePath");
            //WHERE:只对满足后面”条件”的那些行进行操作（查询、更新或删除）。如果不写 WHERE，SQL 就会把这个操作施加到整个表的所有行上
            checkQuery.bind(":filePath", info.filePath);
            //.bind的作用是和占位符做绑定

            if (checkQuery.executeStep())//getColumn() 有一个铁律：在调用 getColumn() 之前，必须确保 executeStep() 返回了 true
            {
                existingId = checkQuery.getColumn("songId").getInt64();
                int64_t existingSize = checkQuery.getColumn("fileSize").getInt64();
                std::string existingTime = checkQuery.getColumn("lastModifiedTime").getString();
                //这里是按 SELECT 中写明的列名读取

                // 文件大小和最后修改时间完全相同 → 视为同一文件，跳过插入
                if (static_cast<int64_t>(info.fileSize) == existingSize
                    && info.lastModifiedTime == existingTime)
                {
                    return;
                }
            }
        }

        // ── 事务开始（RAII(资源获取即初始化)：析构时若未 commit 则自动回滚，阻止插入） ──
        SQLite::Transaction transaction(*db);

        if (existingId >= 0)
        {
            // ── 文件已变更：更新 songs 记录 ──
            SQLite::Statement updateSong(*db,
                "UPDATE songs SET filePath = :filePath, fileSize = :fileSize, "
                "lastModifiedTime = :lastModifiedTime, isMultiStreamFile = :isMultiStreamFile, "
                "duration = :duration, title = :title, artist = :artist, album = :album, albumArtist = :albumArtist, "
                "genre = :genre, trackNumber = :trackNumber, discNumber = :discNumber, year = :year, composer = :composer, "
                "imageHash = :imageHash, bitRate = :bitRate, bitDepth = :bitDepth, sampleRate = :sampleRate, numChannels = :numChannels, codecName = :codecName "
                "WHERE songId = :songId"
            );
            //UPDATE songs 表示要对 songs 表进行更新操作。
            //SET 后面跟着一系列 字段 = :命名参数，表示要把这些字段的值替换成绑定的新值。
            //WHERE 限定只更新那些 songId 等于绑定值的行

            bindSongFields(updateSong);
            updateSong.bind(":songId", existingId);
            updateSong.exec();

        }
        else
        {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(*db,
                "INSERT INTO songs (filePath, fileSize, lastModifiedTime, "
                "isMultiStreamFile, duration, title, artist, album, albumArtist, "
                "genre, trackNumber, discNumber, year, composer, imageHash, bitRate, bitDepth, sampleRate, numChannels, codecName) "
                "VALUES (:filePath, :fileSize, :lastModifiedTime, :isMultiStreamFile, :duration, :title, :artist, :album, :albumArtist, "
                ":genre, :trackNumber, :discNumber, :year, :composer, :imageHash, :bitRate, :bitDepth, :sampleRate, :numChannels, :codecName)"
            );

            bindSongFields(insertSong);
            insertSong.exec();
        }
        // ── 插入/更新完成后重建 nameId 排序 ──
        rebuildNameIds();

        // ── 全部成功，提交事务 ──
        transaction.commit();
        return;
    }
    catch (const SQLite::Exception& e)
    {
        // 事务 RAII 保证：析构时检测到未 commit → 自动 ROLLBACK
        // 数据库恢复到"这首歌完全没存在过"的干净状态
        auto logger = spdlog::get(LogAllID);
        if(logger) logger->error("data update error",e.what());
    }
}

// ============================================================
// getTotalSongCount
// ============================================================
int SongsManage::getTotalSongCount()
{
    if (!db) {
        auto logger = spdlog::get(LogAllID);
        logger->debug("查询歌曲总数阶段发生空指针问题");
        return 0;
    };
    SQLite::Statement query(*db, "SELECT COUNT(*) FROM songs");
    if (query.executeStep())
        return query.getColumn(0).getInt();
    return 0;
}
auto dataLookfor = [](SQLite::Statement& query,std::vector<SongInfo>& result){
    while (query.executeStep())
    {
        SongInfo info;
        info.filePath         = query.getColumn("filePath").getString();
        info.fileSize         = query.getColumn("fileSize").getInt64();
        info.lastModifiedTime = query.getColumn("lastModifiedTime").getString();
        info.isMultiStreamFile = query.getColumn("isMultiStreamFile").getInt() != 0;
        info.duration         = query.getColumn("duration").getDouble();
        info.title            = query.getColumn("title").getString();
        info.artist           = query.getColumn("artist").getString();
        info.album            = query.getColumn("album").getString();
        info.albumArtist      = query.getColumn("albumArtist").getString();
        info.genre            = query.getColumn("genre").getString();
        info.trackNumber      = query.getColumn("trackNumber").getInt();
        info.discNumber       = query.getColumn("discNumber").getInt();
        info.year             = query.getColumn("year").getInt();
        info.composer         = query.getColumn("composer").getString();
        info.imageHash        = query.getColumn("imageHash").getString();
        info.bitRate          = query.getColumn("bitRate").getInt64();
        info.bitDepth         = query.getColumn("bitDepth").getInt();
        info.sampleRate       = query.getColumn("sampleRate").getInt();
        info.numChannels      = query.getColumn("numChannels").getInt();
        info.codecName        = query.getColumn("codecName").getString();
        info.isMusic          = query.getColumn("isMusic").getInt() != 0;
        info.aiGenre          = query.getColumn("aiGenre").getString();
        info.bpm              = query.getColumn("bpm").getInt();
        info.key              = query.getColumn("key").getString();
        info.aiProcessed      = query.getColumn("aiProcessed").getInt() != 0;
        info.isMyLike         = query.getColumn("isMyLike").getInt() != 0;
        info.comment          = query.getColumn("comment").getString();
        info.hadPlayedNum     = query.getColumn("hadPlayedNum").getInt();
        info.nameId           = query.getColumn("nameId").getInt();
        result.push_back(std::move(info));
    }
};
// ============================================================
// getSongsPage
// ============================================================
std::vector<SongInfo> SongsManage::getSongsPageBySongId(int offset, int limit,bool ascending)
{
    std::vector<SongInfo> result;
    if (!db) {
        auto logger = spdlog::get(LogAllID);
        logger->debug("更新歌曲页码阶段发生空指针问题");
        return result;
    }
    std::string sql = "SELECT filePath, fileSize, lastModifiedTime, "
                        "isMultiStreamFile, duration, title, artist, album, albumArtist, "
                        "genre, trackNumber, discNumber, year, composer, imageHash, "
                        "bitRate, bitDepth, sampleRate, numChannels, codecName, "
                        "isMusic, aiGenre, bpm, key, aiProcessed, "
                        "isMyLike, comment, hadPlayedNum, nameId "
                        "FROM songs ORDER BY songId ";
    if(ascending){
        sql +=" ASC ";
    }else{
        sql +=" DESC ";
    }
    sql += " LIMIT :limit OFFSET :offset";
    SQLite::Statement query(*db,sql);

    //OFFSET 的起始行从 0 开始计数
    //DESC：降序排列（Descending）。即时间越大的（越新的）排在最前面。如果写成 ASC，则是升序
    //LIMIT :limit：限制返回的行数。即这一页最多取多少条记录。
    //OFFSET :offset：偏移量（跳过多少行）。即从第几条数据开始取。
    //注意OFFSET这里是行数，也就是单条目数据量，不是偏移的页码数
    query.bind(":limit", limit);
    query.bind(":offset", offset);


    dataLookfor(query, result);

    return result;
}
std::vector<SongInfo> SongsManage::getSongPageByPlayTimes(int offset, int limit,bool ascending)
{
    std::vector<SongInfo> result;
    if (!db) {
        auto logger = spdlog::get(LogAllID);
        logger->debug("按播放次数排序查询歌曲阶段发生空指针问题");
        return result;
    };
    std::string sql = "SELECT filePath, fileSize, lastModifiedTime, "
                        "isMultiStreamFile, duration, title, artist, album, albumArtist, "
                        "genre, trackNumber, discNumber, year, composer, imageHash, "
                        "bitRate, bitDepth, sampleRate, numChannels, codecName, "
                        "isMusic, aiGenre, bpm, key, aiProcessed, "
                        "isMyLike, comment, hadPlayedNum, nameId "
                        "FROM songs ORDER BY hadPlayedNum ";
    if(ascending){
        sql +=" ASC ";
    }else{
        sql +=" DESC ";
    }
    sql += " LIMIT :limit OFFSET :offset";
    SQLite::Statement query(*db,sql);

    query.bind(":limit", limit);
    query.bind(":offset", offset);

    
    dataLookfor(query,result);
    
    return result;
}
std::vector<SongInfo> SongsManage::getSongPageByName(int offset, int limit, bool ascending)
{
    std::vector<SongInfo> result;
    if (!db)
    {
        auto logger = spdlog::get(LogAllID);
        if (logger) logger->debug("按名称排序查询歌曲阶段发生空指针问题");
        return result;
    }

    std::string sql = "SELECT filePath, fileSize, lastModifiedTime, "
                        "isMultiStreamFile, duration, title, artist, album, albumArtist, "
                        "genre, trackNumber, discNumber, year, composer, imageHash, "
                        "bitRate, bitDepth, sampleRate, numChannels, codecName, "
                        "isMusic, aiGenre, bpm, key, aiProcessed, "
                        "isMyLike, comment, hadPlayedNum, nameId "
                        "FROM songs ORDER BY nameId ";
    if (ascending)
        sql += " ASC ";
    else
        sql += " DESC ";
    sql += " LIMIT :limit OFFSET :offset";
    SQLite::Statement query(*db, sql);
    query.bind(":limit", limit);
    query.bind(":offset", offset);

    dataLookfor(query, result);
    return result;
}

std::vector<SongInfo> SongsManage::getSongPage(int offset, int limit, bool ascending, SortMode mode)
{
    switch (mode)
    {
        case SortMode::ByAddTime:  return getSongsPageBySongId(offset, limit, ascending);
        case SortMode::ByName:     return getSongPageByName(offset, limit, ascending);
        case SortMode::ByPlayTimes: return getSongPageByPlayTimes(offset, limit, ascending);
    }
    return {};
}

void SongsManage::rebuildNameIds()
{
    if (!db) return;

    // ── 1. 取出所有 songId 和 title ──
    struct NameEntry { int64_t songId; std::string title; };
    std::vector<NameEntry> entries;
    {
        SQLite::Statement query(*db, "SELECT songId, title FROM songs");
        while (query.executeStep())
        {
            entries.push_back({
                query.getColumn("songId").getInt64(),
                query.getColumn("title").getString()
            });
        }
    }
    if (entries.empty()) return;

    // ── 2. ICU Collator 按 title 排序 ──
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::Collator> coll(
        icu::Collator::createInstance(icu::Locale::getRoot(), status));

    if (U_SUCCESS(status) && coll)
    {
        std::sort(entries.begin(), entries.end(),
            [&](const NameEntry& a, const NameEntry& b)
            {
                UErrorCode err = U_ZERO_ERROR;
                return coll->compareUTF8(
                    icu::StringPiece(a.title),
                    icu::StringPiece(b.title), err) == UCOL_LESS;
            });
    }

    // ── 3. 按排序后的顺序更新 nameId ──
    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        SQLite::Statement update(*db,
            "UPDATE songs SET nameId = :nameId WHERE songId = :songId");
        update.bind(":nameId", i + 1);
        update.bind(":songId", entries[i].songId);
        update.exec();
    }
}

SongsManage::~SongsManage(){

}
