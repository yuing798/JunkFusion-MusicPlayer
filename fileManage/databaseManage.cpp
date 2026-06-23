#include "databaseManage.hpp"
#include "FontAbout/font.h"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <SQLiteCpp/Database.h>
#include <memory>
#include <spdlog/spdlog.h>

SongsManage::SongsManage(){
    auto databaseDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getChildFile("database");
    if(!databaseDir.exists()) databaseDir.createDirectory();

    songsDbFile = databaseDir.getChildFile("songs.db");

    try{
        songsDatabase = std::make_unique<SQLite::Database>(
            songsDbFile.getFullPathName().toStdString(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        createTables(*songsDatabase);
    }catch(const std::exception& e){
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            U("数据库错误"),
            U("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: ") + juce::String(e.what())
        );
        //spdlog:报错
        auto logger = spdlog::get(LogCrashID);
        if(logger) logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: ",e.what());
    }
}

// ============================================================
// createTables
// ============================================================
void SongsManage::createTables(SQLite::Database& db)
{
    db.exec(createSongsTableSQL);
    db.exec(createStreamsTableSQL);
    db.exec(createStreamsIndexSQL);
    //db.exec() 这个函数的全称是 “执行 SQL 语句”，而不是“创建表”
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
bool SongsManage::isSongExists(SQLite::Database& db, const std::string& filePath)
{
    //SQLite::Database:存储数据库连接句柄（一个指向 .db 文件的指针）、连接状态、是否开启事务等管理信息。它是你操作数据库的“总入口”。
    //SQLite::Statement存储预编译好的 SQL 语句模板（比如 SELECT * FROM songs WHERE id = ?）、
    // 绑定的参数值（你填入的 filePath）、以及当前正在读取的那一行数据（执行查询后的结果缓冲区）。
    SQLite::Statement query(db, "SELECT COUNT(*) FROM songs WHERE filePath = ?");
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
int SongsManage::insertSong(SQLite::Database& db, const SongInfo& info)
{
    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&info](SQLite::Statement& stmt) {
        stmt.bind(1,  info.filePath);
        stmt.bind(2,  info.fileName);
        stmt.bind(3,  info.fileSize);
        stmt.bind(4,  info.lastModifiedTime);
        stmt.bind(5,  info.addTime);
        stmt.bind(6,  info.numAudioStreams);
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
        stmt.bind(17, info.extraMetadata);
        stmt.bind(18, info.comment);
        stmt.bind(19, info.imageHash);
    };

    // ── 绑定 streams 表字段的辅助 lambda ──
    auto bindStreamFields = [](SQLite::Statement& stmt, int64_t songId, const SongInfo::stream& s) {
        stmt.bind(1,  songId);
        stmt.bind(2,  s.streamCount);
        stmt.bind(3,  s.bitRate);
        stmt.bind(4,  s.sampleRate);
        stmt.bind(5,  s.numChannels);
        stmt.bind(6,  s.bitDepth);
        stmt.bind(7,  s.codecName);
        stmt.bind(8,  s.isMusic ? 1 : 0);
        stmt.bind(9,  s.aiGenre);
        stmt.bind(10, s.aiMood);
        stmt.bind(11, s.bpm);
        stmt.bind(12, s.key);
        stmt.bind(13, s.aiProcessed ? 1 : 0);
        stmt.bind(14, s.extraMetadata);
    };

    try
    {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(db,
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
                    return static_cast<int>(existingId);
                }
            }
        }

        // ── 事务开始（RAII(资源获取即初始化)：析构时若未 commit 则自动回滚，阻止插入） ──
        SQLite::Transaction transaction(db);

        int64_t songId = 0;

        if (existingId >= 0)
        {
            // ── 文件已变更：先删除旧流记录 ──
            {
                SQLite::Statement delStreams(db, "DELETE FROM streams WHERE songId = ?");
                delStreams.bind(1, existingId);
                delStreams.exec();
            }

            // ── 更新 songs 记录（包含 file_path 以对齐 bindSongFields 的绑定顺序） ──
            SQLite::Statement updateSong(db,
                "UPDATE songs SET filePath = ?, fileName = ?, fileSize = ?, "
                "lastModifiedTime = ?, addTime = ?, numAudioStreams = ?, "
                "duration = ?, title = ?, artist = ?, album = ?, albumArtist = ?, "
                "genre = ?, trackNumber = ?, discNumber = ?, year = ?, composer = ?, "
                "extraMetadata = ?, comment = ?, imageHash = ? "
                "WHERE songId = ?");

            bindSongFields(updateSong);
            updateSong.bind(20, existingId);//这里把id绑在最后一位
            updateSong.exec();

            songId = existingId;
        }
        else
        {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(db,
                "INSERT INTO songs (filePath, fileName, fileSize, lastModifiedTime, "
                "addTime, numAudioStreams, duration, title, artist, album, albumArtist, "
                "genre, trackNumber, discNumber, year, composer, extraMetadata, comment, imageHash) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            bindSongFields(insertSong);
            insertSong.exec();

            songId = db.getLastInsertRowid();
        }

        // ── 插入 streams 记录（新文件和变更文件共用） ──
        {
            SQLite::Statement insertStream(db,
                "INSERT INTO streams (songId, streamCount, bitRate, sampleRate, "
                "numChannels, bitDepth, codecName, isMusic, aiGenre, aiMood, "
                "bpm, key, aiProcessed, extraMetadata) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            for (const auto& stream : info.streams)
            {
                bindStreamFields(insertStream, songId, stream);
                insertStream.exec();
                insertStream.reset();
                // insertStream.clearBindings();
            }
        }

        // ── 全部成功，提交事务 ──
        transaction.commit();
        return static_cast<int>(songId);
    }
    catch (const SQLite::Exception&)
    {
        // 事务 RAII 保证：析构时检测到未 commit → 自动 ROLLBACK
        // 数据库恢复到"这首歌完全没存在过"的干净状态
        return -1;
    }
}
