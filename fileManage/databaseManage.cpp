#include "databaseManage.hpp"

// ============================================================
// createTables
// ============================================================
void createTables(SQLite::Database& db)
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
bool isSongExists(SQLite::Database& db, const std::string& filePath)
{
    SQLite::Statement query(db, "SELECT COUNT(*) FROM songs WHERE file_path = ?");
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
}

// ============================================================
// insertSong
// ============================================================
int insertSong(SQLite::Database& db, const SongInfo& info)
{
    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&info](SQLite::Statement& stmt) {
        stmt.bind(1,  info.filePath);
        stmt.bind(2,  info.fileName);
        stmt.bind(3,  static_cast<int64_t>(info.fileSize));
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
                "SELECT song_id, file_size, last_modified_time FROM songs WHERE file_path = ?");
            checkQuery.bind(1, info.filePath);

            if (checkQuery.executeStep())
            {
                existingId = checkQuery.getColumn(0).getInt64();
                int64_t existingSize = checkQuery.getColumn(1).getInt64();
                std::string existingTime = checkQuery.getColumn(2).getString();

                // 文件大小和最后修改时间完全相同 → 视为同一文件，跳过插入
                if (static_cast<int64_t>(info.fileSize) == existingSize
                    && info.lastModifiedTime == existingTime)
                {
                    return static_cast<int>(existingId);
                }
            }
        }

        // ── 事务开始（RAII：析构时若未 commit 则自动 ROLLBACK） ──
        SQLite::Transaction transaction(db);

        int64_t songId = 0;

        if (existingId >= 0)
        {
            // ── 文件已变更：先删除旧流记录 ──
            {
                SQLite::Statement delStreams(db, "DELETE FROM streams WHERE song_id = ?");
                delStreams.bind(1, existingId);
                delStreams.exec();
            }

            // ── 更新 songs 记录（包含 file_path 以对齐 bindSongFields 的绑定顺序） ──
            SQLite::Statement updateSong(db,
                "UPDATE songs SET file_path = ?, file_name = ?, file_size = ?, "
                "last_modified_time = ?, add_time = ?, num_audio_streams = ?, "
                "duration = ?, title = ?, artist = ?, album = ?, album_artist = ?, "
                "genre = ?, track_number = ?, disc_number = ?, year = ?, composer = ?, "
                "extra_metadata = ?, comment = ?, image_hash = ? "
                "WHERE song_id = ?");

            bindSongFields(updateSong);
            updateSong.bind(20, existingId);
            updateSong.exec();

            songId = existingId;
        }
        else
        {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(db,
                "INSERT INTO songs (file_path, file_name, file_size, last_modified_time, "
                "add_time, num_audio_streams, duration, title, artist, album, album_artist, "
                "genre, track_number, disc_number, year, composer, extra_metadata, comment, image_hash) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            bindSongFields(insertSong);
            insertSong.exec();

            songId = db.getLastInsertRowid();
        }

        // ── 插入 streams 记录（新文件和变更文件共用） ──
        {
            SQLite::Statement insertStream(db,
                "INSERT INTO streams (song_id, stream_count, bit_rate, sample_rate, "
                "num_channels, bit_depth, codec_name, is_music, ai_genre, ai_mood, "
                "bpm, key, ai_processed, extra_metadata) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

            for (const auto& stream : info.streams)
            {
                bindStreamFields(insertStream, songId, stream);
                insertStream.exec();
                insertStream.reset();
                insertStream.clearBindings();
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
