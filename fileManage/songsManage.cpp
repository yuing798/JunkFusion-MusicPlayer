#include "songsManage.hpp"
// #include "FontAbout/font.h"
#include "BridgeNames.h"
#include "constants.h"
#include "songsModel.hpp"
#include "fileUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <unicode/coll.h>
#include <unicode/locid.h>
#include <unicode/stringpiece.h>
#include <vector>

SongsManage::SongsManage()
:songDb(nullptr){
    auto songsDbFile = databaseDirId.getChildFile("song.db");
    auto imageDbFile = databaseDirId.getChildFile("songImage.db");

    try
    {
        songDb = std::make_unique<SQLite::Database>(
            songsDbFile.getFullPathName().toStdString(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        songDb->exec(createSongsTableSQL);
        songDb->exec(createNameIdIndexSQL);

        songImageDb = std::make_unique<SQLite::Database>(
            imageDbFile.getFullPathName().toStdString(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );
        songImageDb->exec(createSongImageTableSQL);
    }
    catch (const std::exception& e)
    {
        // 数据库初始化失败 → songDb 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogAllID);
        if (logger) logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}", e.what());
    }
}

// ============================================================
// isSongExists
// ============================================================
bool SongsManage::isSongExists(const std::string& filePath)
{
    if (!songDb) return false;
    SQLite::Statement query(*songDb, "SELECT COUNT(*) FROM songs WHERE filePath = :filePath");
    query.bind(":filePath", filePath);
    query.executeStep();
    //执行查询：数据库跑去找数据。
    return query.getColumn(0).getInt() > 0;
    //COUNT(*) 没有列名，所以仍然用数字索引（从 0 开始）
}

// ============================================================
// insertSong
// ============================================================
bool SongsManage::insertSong(const SongInfo& info)
{
    if(!songDb) return false;
    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&info](SQLite::Statement& stmt) {
        // ── 必选字段 ──
        stmt.bind(":filePath",         info.filePath);
        stmt.bind(":fileSize",         static_cast<int64_t>(info.fileSize));
        stmt.bind(":lastModifiedTime", info.lastModifiedTime);
        stmt.bind(":isMultiStreamFile",info.isMultiStreamFile);
        stmt.bind(":duration",         info.duration);
        stmt.bind(":title",            info.title);

        // ── 可选 string 字段：有值则绑定，无值则绑定 NULL ──
        auto bindOptStr = [&](const char* name, const std::optional<std::string>& v) {
            if (v.has_value()) stmt.bind(name, v.value());
            else               stmt.bind(name);  // 无第二个参数 → SQL NULL
        };
        // ── 可选 int 字段 ──
        auto bindOptInt = [&](const char* name, const std::optional<int>& v) {
            if (v.has_value()) stmt.bind(name, v.value());
            else               stmt.bind(name);
        };

        bindOptStr(":artist",      info.artist);
        bindOptStr(":album",       info.album);
        bindOptStr(":albumArtist", info.albumArtist);
        bindOptStr(":genre",       info.genre);
        bindOptInt(":trackNumber", info.trackNumber);
        bindOptInt(":discNumber",  info.discNumber);
        bindOptInt(":year",        info.year);
        bindOptStr(":composer",    info.composer);
        bindOptStr(":imageHash",   info.imageHash);
        bindOptStr(":codecName",   info.codecName);

        // ── FFmpeg 必选 int 字段 ──
        stmt.bind(":bitRate",     info.bitRate);
        stmt.bind(":bitDepth",    info.bitDepth);
        stmt.bind(":sampleRate",  info.sampleRate);
        stmt.bind(":numChannels", info.numChannels);
    };

    try
    {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(*songDb,
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = :filePath");
            checkQuery.bind(":filePath", info.filePath);

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
                    return false;
                }
            }
        }

        // ── 事务开始（RAII(资源获取即初始化)：析构时若未 commit 则自动回滚，阻止插入） ──
        SQLite::Transaction transaction(*songDb);

        if (existingId >= 0)
        {
            // ── 文件已变更：更新 songs 记录 ──
            SQLite::Statement updateSong(*songDb,
                "UPDATE songs SET filePath = :filePath, fileSize = :fileSize, "
                "lastModifiedTime = :lastModifiedTime, isMultiStreamFile = :isMultiStreamFile, "
                "duration = :duration, title = :title, artist = :artist, album = :album, albumArtist = :albumArtist, "
                "genre = :genre, trackNumber = :trackNumber, discNumber = :discNumber, year = :year, composer = :composer, "
                "imageHash = :imageHash, bitRate = :bitRate, bitDepth = :bitDepth, "
                "sampleRate = :sampleRate, numChannels = :numChannels, codecName = :codecName "
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
            SQLite::Statement insertSong(*songDb,
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
        return true;
    }
    catch (const SQLite::Exception& e)
    {
        // 事务 RAII 保证：析构时检测到未 commit → 自动 ROLLBACK
        // 数据库恢复到"这首歌完全没存在过"的干净状态
        auto logger = spdlog::get(LogAllID);
        if(logger) logger->error("data update error",e.what());
        return false;
    }
}

// ============================================================
// getTotalSongCount
// ============================================================
std::optional<int> SongsManage::getTotalSongCount()
{
    if (!songDb) {
        auto logger = spdlog::get(LogAllID);
        logger->debug("查询歌曲总数阶段发生空指针问题");
        return std::nullopt;
    };
    SQLite::Statement query(*songDb, "SELECT COUNT(*) FROM songs");
    if (query.executeStep())
        return query.getColumn(0).getInt();
    return std::nullopt;
}
auto dataLookfor = [](SQLite::Statement& query,std::vector<SongInfo>& result){
    // ── 辅助：读取可能为 NULL 的 string 列 → std::optional<std::string> ──
    auto optStrCol = [&](const char* colName) -> std::optional<std::string> {
        auto col = query.getColumn(colName);
        if (col.isNull()) return std::nullopt;
        std::string s = col.getString();
        if(s.empty()){
            return std::nullopt;
        }else{
            return s;
        }
    };
    // ── 辅助：读取可能为 NULL 的 int 列 → std::optional<int> ──
    auto optIntCol = [&](const char* colName) -> std::optional<int> {
        if (query.getColumn(colName).isNull()) return std::nullopt;
        return query.getColumn(colName).getInt();
    };

    while (query.executeStep())
    {
        SongInfo info;
        info.songId           = query.getColumn("songId").getInt64();
        info.filePath         = query.getColumn("filePath").getString();
        info.fileSize         = query.getColumn("fileSize").getInt64();
        info.lastModifiedTime = query.getColumn("lastModifiedTime").getString();
        info.isMultiStreamFile = query.getColumn("isMultiStreamFile").getInt() != 0;
        info.duration         = query.getColumn("duration").getDouble();

        // ── 标签 ──
        info.title            = query.getColumn("title").getString();
        info.artist           = optStrCol("artist");
        info.album            = optStrCol("album");
        info.albumArtist      = optStrCol("albumArtist");
        info.genre            = optStrCol("genre");
        info.trackNumber      = optIntCol("trackNumber");
        info.discNumber       = optIntCol("discNumber");
        info.year             = optIntCol("year");
        info.composer         = optStrCol("composer");
        info.imageHash        = optStrCol("imageHash");

        // ── FFmpeg 解码层 ──
        info.bitRate          = query.getColumn("bitRate").getInt64();
        info.bitDepth         = query.getColumn("bitDepth").getInt();
        info.sampleRate       = query.getColumn("sampleRate").getInt();
        info.numChannels      = query.getColumn("numChannels").getInt();
        info.codecName        = optStrCol("codecName");

        // ── AI 分析 ──
        info.isMusic          = query.getColumn("isMusic").getInt() != 0;
        info.aiGenre          = optStrCol("aiGenre");
        info.bpm              = optIntCol("bpm");
        info.key              = optStrCol("key");
        info.aiProcessed      = query.getColumn("aiProcessed").getInt() != 0;

        // ── 用户信息 ──
        info.isMyLike         = query.getColumn("isMyLike").getInt() != 0;
        info.comment          = optStrCol("comment");
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
    if (!songDb) {
        auto logger = spdlog::get(LogAllID);
        logger->debug("更新歌曲页码阶段发生空指针问题");
        return result;
    }
    std::string sql = "SELECT songId, filePath, fileSize, lastModifiedTime, "
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
    SQLite::Statement query(*songDb,sql);

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
    if (!songDb) {
        auto logger = spdlog::get(LogAllID);
        logger->debug("按播放次数排序查询歌曲阶段发生空指针问题");
        return result;
    };
    std::string sql = "SELECT songId, filePath, fileSize, lastModifiedTime, "
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
    SQLite::Statement query(*songDb,sql);

    query.bind(":limit", limit);
    query.bind(":offset", offset);

    
    dataLookfor(query,result);
    
    return result;
}
std::vector<SongInfo> SongsManage::getSongPageByName(int offset, int limit, bool ascending)
{
    std::vector<SongInfo> result;
    if (!songDb)
    {
        auto logger = spdlog::get(LogAllID);
        if (logger) logger->debug("按名称排序查询歌曲阶段发生空指针问题");
        return result;
    }

    std::string sql = "SELECT songId, filePath, fileSize, lastModifiedTime, "
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
    SQLite::Statement query(*songDb, sql);
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
    if (!songDb) return;

    // ── 1. 取出所有 songId 和 title ──
    struct NameEntry { int64_t songId; std::string title; };
    std::vector<NameEntry> entries;
    {
        SQLite::Statement query(*songDb, "SELECT songId, title FROM songs");
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
        SQLite::Statement update(*songDb,
            "UPDATE songs SET nameId = :nameId WHERE songId = :songId");
        update.bind(":nameId", i + 1);
        update.bind(":songId", entries[i].songId);
        update.exec();
    }
}

bool SongsManage::reverseMyLike(int64_t id){
    
    try{
        SQLite::Statement sql(*songDb,
            "UPDATE songs SET isMyLike = 1 - isMyLike WHERE songId = :songId");
        sql.bind(":songId",id);

        int rowAffected = sql.exec();//（数据变更语句）：返回受影响的行数
        if(rowAffected > 0) return true;
    }catch(...){
        auto logger{spdlog::get(LogSchedulerID)};
        logger->critical("[我喜欢]状态更新失败，请重试");
        return false;
    }
}

SongsManage::~SongsManage(){

}

juce::WebBrowserComponent::Options songsManageBuilder::buildOptions(const juce::WebBrowserComponent::Options& initial){
    return initial
    .withNativeFunction(//得到总歌曲数目
        B_getAllSongCount::name,//得到总歌曲数目
        [](const auto& args,auto complete){
            auto count{SongsManage::getInstance().getTotalSongCount()};
            auto obj{new juce::DynamicObject()};
            if(count.has_value()){
                obj->setProperty(B_getAllSongCount::count,count.value());
                complete(obj);
                return;
            }else{
                obj->setProperty(B_getAllSongCount::count,0);
                complete(obj);
                return ;
            }
        }
    ).withNativeFunction(B_toggleMyLike::name,//将我喜欢的歌曲状态翻转
        [](
            const juce::Array<juce::var>& args,
            juce::WebBrowserComponent::NativeFunctionCompletion complete
        ){
            int64_t id{0};
            if(args.size()>=1) id = args[0];
            if(SongsManage::getInstance().reverseMyLike(id)){
                complete(juce::var());
                return;
            }else{
                auto error{new juce::DynamicObject()};
                error->setProperty(B_event::fullError,utf8("[我喜欢]状态更新失败，请重试"));
                complete(juce::var(error));
                return;
            }
        }
    ).withNativeFunction(//得到单页的歌曲信息
        B_refreshAllMusicSongs::name,//参数：当前页码,升降序，排序方法，
        [](
            const juce::Array<juce::var>& args,auto complete
        ){
            auto sortMode{SongsManage::SortMode::ByAddTime};
            int sortWay = args[0][B_refreshAllMusicSongs::sortMode];
            bool isAscending = args[0][B_refreshAllMusicSongs::isAscending];
            int targetPage = args[0][B_refreshAllMusicSongs::page];
            switch (sortWay) {
                case 0:
                    sortMode = SongsManage::SortMode::ByAddTime;
                    break;
                case 1:
                    sortMode = SongsManage::SortMode::ByName;
                    break;
                case 2:
                    sortMode = SongsManage::SortMode::ByPlayTimes;
                    break;
                default:
                    sortMode = SongsManage::SortMode::ByAddTime;
                    break;
            }
            auto results{SongsManage::getInstance().getSongPage(
                targetPage,
                std::stoi(B_other::numRows4SinglePage),
                isAscending,
                sortMode
            )};//vector可以为空，所以不需要std::optional进行检测
            if(results.empty()){
                auto obj{new juce::DynamicObject()};
                obj->setProperty(B_refreshAllMusicSongs::error,-1);
                complete(obj);
                return ;
            }else{
                complete(SongInfo::vector2VarArray(results));
                return ;
            }
        }
    );
}
