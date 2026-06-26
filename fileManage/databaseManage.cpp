#include "databaseManage.hpp"
#include "FontAbout/font.h"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <SQLiteCpp/Database.h>
#include <memory>
#include <spdlog/spdlog.h>
auto logger = spdlog::get(LogAllID);

SongsManage::SongsManage()
{
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
        
        if (logger) logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}", e.what());
    }
}

// ============================================================
// createTables
// ============================================================
void SongsManage::createTables()
{
    db->exec(createSongsTableSQL);
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
    SQLite::Statement query(*db, "SELECT COUNT(*) FROM songs WHERE filePath = ?");
    //告诉数据库“我要数一下，songs 表里有多少行的 file_path 等于后面那个问号”。
    //这个 ? 是一个“空位”，专门留给后面的 C++ 变量来填的
    //目的：防止 SQL 注入攻击
    //SQLite 会先把带 ? 的 SQL 编译成“执行计划”，然后只替换 ? 的值。如果你要循环插入一万首歌，这种写法比拼字符串快得多。
    query.bind(1, filePath);
    //把函数传进来的 filePath（比如 "C:\Music\Adele.mp3"）塞到刚才那个 ? 的位置上
    //参数绑定看“问号的位置”，列读取看“SELECT 写的顺序”
    //而在SQLite 中，? 占位符的索引从 1 开始，所以bind函数这里填1
    query.executeStep();
    //执行查询：数据库跑去找数据。
    return query.getColumn(0).getInt() > 0;
    //getColumn 数 SELECT 的列（从 0 开始）
}

// ============================================================
// insertSong
// ============================================================
void SongsManage::insertSong(const SongInfo& info)
{
    if (!db) return;
    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&info](SQLite::Statement& stmt) {
        stmt.bind(1,  info.filePath);
        stmt.bind(2,  info.fileName);
        stmt.bind(3,  info.fileSize);
        stmt.bind(4,  info.lastModifiedTime);
        stmt.bind(5,  info.addTime);
        stmt.bind(6,  info.isMultiStreamFile);
        stmt.bind(7,  info.duration);
        stmt.bind(8,  info.title);
        stmt.bind(9,  info.artist);
        stmt.bind(10, info.album);
        stmt.bind(11, info.albumArtist);
        stmt.bind(12, info.genre);
        stmt.bind(13, info.trackNumber);
        stmt.bind(14, info.discNumber);
        stmt.bind(15, info.year);
        stmt.bind(16, info.composer);
        stmt.bind(17, info.imageHash);
        stmt.bind(18,info.bitRate);
        stmt.bind(19,info.bitDepth);
        stmt.bind(20,info.sampleRate);
        stmt.bind(21,info.numChannels);
        stmt.bind(22,info.codecName);
    };

    try
    {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(*db,
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = ?");
            checkQuery.bind(1, info.filePath);

            if (checkQuery.executeStep())//getColumn() 有一个铁律：在调用 getColumn() 之前，必须确保 executeStep() 返回了 true
            {
                existingId = checkQuery.getColumn(0).getInt64();
                int64_t existingSize = checkQuery.getColumn(1).getInt64();
                std::string existingTime = checkQuery.getColumn(2).getString();
                //这里的aIndex是相对于上面写的SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = ?
                //而不是表中的顺序

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
                "UPDATE songs SET filePath = ?, fileName = ?, fileSize = ?, "
                "lastModifiedTime = ?, addTime = ?, isMultiStreamFile = ?, "
                "duration = ?, title = ?, artist = ?, album = ?, albumArtist = ?, "
                "genre = ?, trackNumber = ?, discNumber = ?, year = ?, composer = ?, "
                "imageHash = ?, bitRate = ?, bitDepth = ?,sampleRate = ?,numChannels = ?, codecName = ? "
                "WHERE songId = ?");

            bindSongFields(updateSong);
            updateSong.bind(23, existingId);//这里把id绑在最后一位
            updateSong.exec();

        }
        else
        {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(*db,
                "INSERT INTO songs (filePath, fileName, fileSize, lastModifiedTime, "
                "addTime, isMultiStreamFile, duration, title, artist, album, albumArtist, "
                "genre, trackNumber, discNumber, year, composer, imageHash, bitRate, bitDepth, sampleRate, numChannels, codecName) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            bindSongFields(insertSong);
            insertSong.exec();
        }
        // ── 全部成功，提交事务 ──
        transaction.commit();
        return;
    }
    catch (const SQLite::Exception& e)
    {
        // 事务 RAII 保证：析构时检测到未 commit → 自动 ROLLBACK
        // 数据库恢复到"这首歌完全没存在过"的干净状态
        auto logger = spdlog::get(LogSchedulerID);
        if(logger) logger->error("data update error",e.what());
    }
}

// ============================================================
// getTotalSongCount
// ============================================================
int SongsManage::getTotalSongCount()
{
    if (!db) {
        logger->debug("查询歌曲总数阶段发生空指针问题");
        return 0;
    };
    SQLite::Statement query(*db, "SELECT COUNT(*) FROM songs");
    if (query.executeStep())
        return query.getColumn(0).getInt();
    return 0;
}

// ============================================================
// getSongsPage
// ============================================================
std::vector<SongInfo> SongsManage::getSongsPage(int offset, int limit)
{
    std::vector<SongInfo> result;
    if (!db) {
        logger->debug("更新歌曲页码阶段发生空指针问题");
        return result;
    };
    SQLite::Statement query(*db,
        "SELECT filePath, fileName, fileSize, lastModifiedTime, addTime, "
        "isMultiStreamFile, duration, title, artist, album, albumArtist, "
        "genre, trackNumber, discNumber, year, composer, imageHash, "
        "bitRate, bitDepth, sampleRate, numChannels, codecName, "
        "isMusic, aiGenre, aiMood, bpm, key, aiProcessed, "
        "isMyLike, comment, hadPlayedNum "
        "FROM songs ORDER BY addTime DESC LIMIT ? OFFSET ?");
    query.bind(1, limit);
    query.bind(2, offset);

    while (query.executeStep())
    {
        SongInfo info;
        info.filePath         = query.getColumn(0).getString();
        info.fileName         = query.getColumn(1).getString();
        info.fileSize         = query.getColumn(2).getInt64();
        info.lastModifiedTime = query.getColumn(3).getString();
        info.addTime          = query.getColumn(4).getString();
        info.isMultiStreamFile = query.getColumn(5).getInt() != 0;
        info.duration         = query.getColumn(6).getDouble();
        info.title            = query.getColumn(7).getString();
        info.artist           = query.getColumn(8).getString();
        info.album            = query.getColumn(9).getString();
        info.albumArtist      = query.getColumn(10).getString();
        info.genre            = query.getColumn(11).getString();
        info.trackNumber      = query.getColumn(12).getInt();
        info.discNumber       = query.getColumn(13).getInt();
        info.year             = query.getColumn(14).getInt();
        info.composer         = query.getColumn(15).getString();
        info.imageHash        = query.getColumn(16).getString();
        info.bitRate          = query.getColumn(17).getInt();
        info.bitDepth         = query.getColumn(18).getInt();
        info.sampleRate       = query.getColumn(19).getInt();
        info.numChannels      = query.getColumn(20).getInt();
        info.codecName        = query.getColumn(21).getString();
        info.isMusic          = query.getColumn(22).getInt() != 0;
        info.aiGenre          = query.getColumn(23).getString();
        info.aiMood           = query.getColumn(24).getString();
        info.bpm              = query.getColumn(25).getInt();
        info.key              = query.getColumn(26).getString();
        info.aiProcessed      = query.getColumn(27).getInt() != 0;
        info.isMyLike         = query.getColumn(28).getInt() != 0;
        info.comment          = query.getColumn(29).getString();
        info.hadPlayedNum     = query.getColumn(30).getInt();
        result.push_back(std::move(info));
    }
    return result;
}

SongsManage::~SongsManage(){

}
