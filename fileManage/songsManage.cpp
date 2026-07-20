#include "songsManage.hpp"
#include "BridgeNames.h"
#include "constants.h"
#include "otherUtils.hpp"
#include "dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unicode/coll.h>
#include <unicode/locid.h>
#include <unicode/stringpiece.h>
#include <utility>
#include <vector>
#include "./dbManager.hpp"

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/dict.h>
    #include <libavutil/samplefmt.h>
}

SongsManage::SongsManage(SQLite::Database& d)
:db(d){
    try
    {
        db.exec(createSongsTableSQL);
        db.exec(createNameIdIndexSQL);
    }
    catch (const std::exception& e)
    {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogAllID);
        if (logger) logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}", e.what());
    }
}

bool SongsManage::insertSong(const juce::File& path)
{
    
    SongInfo info{};
    info.filePath = path.getFullPathName().toStdString();
    info.fileSize = path.getSize();
    info.lastModifiedTime = path.getLastModificationTime().toString(true, true).toStdString();

    auto logger{spdlog::get(LogSchedulerID)};

    //ffmpeg解码层信息
    int result{0};//解码层结果，一般成功返回零
    AVFormatContext* inputContext{nullptr};
    result = avformat_open_input(&inputContext, info.filePath.c_str(), nullptr, nullptr);
    if(result!=0){
        //非多媒体文件也会返回AVERROR
        //SPDLOG:记录多媒体文件无法打开文件或者打开的是非多媒体文件
        if(logger) logger->warn("多媒体文件无法打开或者打开的是非多媒体文件:{}",ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return false;
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if(result<0){
        //SPDLOG:无法找到流信息
        if(logger) logger->error("无法找到该文件的流信息:{}",ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return false;
    }

    // 时长（秒）
    if (inputContext->duration != AV_NOPTS_VALUE)
    {
        info.duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
    }//每条流的时长都一样

    auto currentIndex{av_find_best_stream(
        inputContext, 
        AVMEDIA_TYPE_AUDIO, 
        -1, 
        -1, 
        nullptr, 
        0
    )};

    auto*           pAudioStream = inputContext->streams[currentIndex];
    auto*  decoderPar      = pAudioStream->codecpar;
    info.codecID = decoderPar->codec_id;
    info.codecName = avcodec_get_name(decoderPar->codec_id);

    // 比特率（kbps）——先取编码器报告值，缺失时用文件大小估算
    info.bitRate = decoderPar->bit_rate / 1000;
    if (info.bitRate <= 0 && info.duration > 0.0 && info.fileSize > 0)
    {
        info.bitRate = static_cast<int>(
            info.fileSize * 8.0 / info.duration / 1000.0);
    }

    // 采样率（Hz）
    info.sampleRate = decoderPar->sample_rate;

    // 通道数
    info.numChannels = decoderPar->ch_layout.nb_channels;

    // 位深 —— 仅 PCM 编码有意义，压缩编码 bits_per_coded_sample 为其解码位深
    int bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(decoderPar->format));
    if (bytesPerSample > 0)
    {
        info.bitDepth = bytesPerSample * 8;
    }
    else
    {
        info.bitDepth = decoderPar->bits_per_coded_sample;
    }
    //提取文件层面的标签数据
    AVDictionary*   pTags = inputContext->metadata;
    AVDictionaryEntry* pEntry = nullptr;

    if ((pEntry = av_dict_get(pTags, "title",       nullptr, 0))){
        info.title = pEntry->value;
    }else{
        info.title = juce::File(info.filePath).getFileNameWithoutExtension().toStdString();
    }
    auto safeToInt = [](const char* str) -> int{
        try { return std::stoi(str); }
        catch (...) { return 0; }
    };

    if ((pEntry = av_dict_get(pTags, "artist",       nullptr, 0)))
        info.artist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "album",        nullptr, 0)))
        info.album = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "album_artist", nullptr, 0)))
        info.albumArtist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "genre",        nullptr, 0)))
        info.genre = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "track",        nullptr, 0)))
        info.trackNumber = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "disc",         nullptr, 0)))
        info.discNumber = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "date",         nullptr, 0)))
        info.year = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "composer",     nullptr, 0)))
        info.composer = pEntry->value;

    //这里进行封面提取
    AVPacket coverPacket;
    std::string hash;
    coverPacket.data = nullptr;
    coverPacket.size = 0;
    SHA1 sha1;
    for(size_t i=0; i<inputContext->nb_streams; i++){
        auto* stream{inputContext->streams[i]};
        auto type{stream->codecpar->codec_type};
        if(type == AVMEDIA_TYPE_ATTACHMENT || (type == AVMEDIA_TYPE_VIDEO && (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))){
            coverPacket = stream->attached_pic;
            break;
        }

    }//这里只是做出了图片编码，但是没有解码，不能获得宽度和高度

    if(coverPacket.data && coverPacket.size > 0){
        do{
            hash = sha1(coverPacket.data,coverPacket.size);

            juce::File hashImageDir{songImageDirId.getChildFile(hash)};
            //直接用哈希值作为文件夹名，所有该图片相关的缓存文件都放在同一个文件夹中

            if(hashImageDir.exists()){
                break;
            }else{
                hashImageDir.createDirectory();
            }//如果这个目录已经存在，直接退出，避免保存两个相同图片

            juce::File originalFile{hashImageDir.getChildFile("original.jpg")};

            juce::FileOutputStream outputStream(originalFile);
            if(outputStream.openedOk()){
                outputStream.write(coverPacket.data, coverPacket.size);
                outputStream.flush();
            }

        }while(0);
    }
    avformat_close_input(&inputContext);

    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&info,hash](SQLite::Statement& stmt) {
        // ── 必选字段 ──
        stmt.bind(":filePath",         info.filePath);
        stmt.bind(":fileSize",         static_cast<int64_t>(info.fileSize));
        stmt.bind(":lastModifiedTime", info.lastModifiedTime);
        stmt.bind(":duration",         info.duration);
        stmt.bind(":title",            info.title);
        // ── FFmpeg 必选 int 字段 ──
        stmt.bind(":bitRate",     info.bitRate);
        stmt.bind(":bitDepth",    info.bitDepth);
        stmt.bind(":sampleRate",  info.sampleRate);
        stmt.bind(":numChannels", info.numChannels);

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
        bindOptStr(":codecName",   info.codecName);
        bindOptStr(":hash", hash);
    };

    try
    {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(
                db,
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = :filePath"
            );
            checkQuery.bind(":filePath", info.filePath);

            if (checkQuery.executeStep())
            {
                existingId = checkQuery.getColumn("songId").getInt64();
                int64_t existingSize = checkQuery.getColumn("fileSize").getInt64();
                std::string existingTime = checkQuery.getColumn("lastModifiedTime").getString();

                // 文件大小和最后修改时间完全相同 → 视为同一文件，跳过插入
                if (static_cast<int64_t>(info.fileSize) == existingSize
                    && info.lastModifiedTime == existingTime)
                {
                    return false;
                }
            }
        }

        // ── 事务开始（RAII(资源获取即初始化)：析构时若未 commit 则自动回滚，阻止插入） ──
        SQLite::Transaction transaction(db);

        if (existingId >= 0)
        {
            // ── 文件已变更：更新 songs 记录 ──
            SQLite::Statement updateSong(db,
                "UPDATE songs SET filePath = :filePath, fileSize = :fileSize, "
                "lastModifiedTime = :lastModifiedTime, "
                "duration = :duration, title = :title, artist = :artist, album = :album, albumArtist = :albumArtist, "
                "genre = :genre, trackNumber = :trackNumber, discNumber = :discNumber, year = :year, composer = :composer, "
                "bitRate = :bitRate, bitDepth = :bitDepth, hash = :hash, "
                "sampleRate = :sampleRate, numChannels = :numChannels, codecName = :codecName "
                "WHERE songId = :songId"
            );

            bindSongFields(updateSong);
            updateSong.bind(":songId", existingId);
            updateSong.exec();

        }
        else
        {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(db,
                "INSERT INTO songs (filePath, fileSize, lastModifiedTime, "
                "duration, title, artist, album, albumArtist, "
                "genre, trackNumber, discNumber, year, composer, bitRate, bitDepth, sampleRate, numChannels, codecName, hash) "
                "VALUES (:filePath, :fileSize, :lastModifiedTime, :duration, :title, :artist, :album, :albumArtist, "
                ":genre, :trackNumber, :discNumber, :year, :composer, :bitRate, :bitDepth, :sampleRate, :numChannels, :codecName, :hash)"
            );

            bindSongFields(insertSong);
            insertSong.exec();
        }
        // ── 插入/更新完成后重建 nameId 排序 ──
        rebuildNameIds();

        // ── 全部成功，提交事务 ──
        transaction.commit();
    }
    catch (const SQLite::Exception& e)
    {
        if(logger) logger->error("insert song error:{}",e.what());
        return false;
    }

    return true;
}

std::optional<int> SongsManage::getTotalSongCount()
{
    SQLite::Statement query(db, "SELECT COUNT(*) FROM songs");
    if (query.executeStep())//exec返回的是受影响的函数，executeStep返回是否还有需要执行的行
        return query.getColumn(0).getInt();
    return std::nullopt;
}

std::vector<SongInfo> SongsManage::getSongPage(int page, bool ascending, SortMode mode)
{

    std::vector<SongInfo> result;

    std::string sql = R"(
        SELECT songId, filePath, fileSize, lastModifiedTime, 
        duration, title, artist, album, albumArtist, 
        genre, trackNumber, discNumber, year, composer, 
        bitRate, bitDepth, sampleRate, numChannels, codecName, 
        aiGenre, bpm, key, aiProcessed, 
        isMyLike, comment, hadPlayedNum, nameId 
        FROM songs 
        ORDER BY 
    )";//按照顺序查询的sql语句，后面接上排序方式;
    switch(mode){
        case SongsManage::SortMode::ByAddTime://添加进应用中的时间和songId一样都是单调递增的
            sql += " songId ";
            break;
        case SongsManage::SortMode::ByName:
            sql += " nameId ";
            break;
        case SongsManage::SortMode::ByPlayTimes:
            sql += " hadPlayedNum " ;
            break;
    }
    if (ascending)
        sql += " ASC ";
    else
        sql += " DESC ";
    sql += " LIMIT :limit OFFSET :offset";
    SQLite::Statement query(db, sql);
    query.bind(":limit", traditionalPageRows);
    query.bind(":offset", traditionalPageRows * page);

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
        SongInfo info{};
        info.songId           = query.getColumn("songId").getInt64();
        info.filePath         = query.getColumn("filePath").getString();
        info.fileSize         = query.getColumn("fileSize").getInt64();
        info.lastModifiedTime = query.getColumn("lastModifiedTime").getString();
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

        // ── FFmpeg 解码层 ──
        info.bitRate          = query.getColumn("bitRate").getInt64();
        info.bitDepth         = query.getColumn("bitDepth").getInt();
        info.sampleRate       = query.getColumn("sampleRate").getInt();
        info.numChannels      = query.getColumn("numChannels").getInt();
        info.codecName        = optStrCol("codecName");

        // ── AI 分析 ──
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

    return result;
}

void SongsManage::rebuildNameIds()
{

    // ── 1. 取出所有 songId 和 title ──
    struct NameEntry { int64_t songId; std::string title; };
    std::vector<NameEntry> entries;
    {
        SQLite::Statement query(db, "SELECT songId, title FROM songs");
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
        SQLite::Statement update(db,
            "UPDATE songs SET nameId = :nameId WHERE songId = :songId");
        update.bind(":nameId", i + 1);
        update.bind(":songId", entries[i].songId);
        update.exec();
    }
}

bool SongsManage::reverseMyLike(int64_t id){
    
    try{
        SQLite::Statement sql(db,
            "UPDATE songs SET isMyLike = 1 - isMyLike WHERE songId = :songId");
        sql.bind(":songId",id);

        if(sql.exec() > 0){
            return true;
        }else{
            return false;
        }
    }catch(const SQLite::Exception& e){
        auto logger{spdlog::get(LogUiID)};
        logger->error("[我喜欢]状态更新失败，请重试:{}",e.what());
        return false;
    }
}

std::string SongsManage::getImageHashBySongId(int64_t id){
    try{
        SQLite::Statement sql(
            db,
            "SELECT hash FROM songs WHERE songs.songId = :songId"
        );
        sql.bind(":songId",id);
        if(sql.executeStep()){
            return sql.getColumn("hash").getString();
        }
    }catch(const SQLite::Exception& e){
        auto logger =  spdlog::get(LogUiID);
        logger->error("获取歌曲封面哈希值失败:{}",e.what());
    }
    return "";
}

void SongsManage::saveComment(juce::String text,int64_t songId){
    try{
        SQLite::Statement sql(
            db,
            "UPDATE songs SET comment = :comment WHERE somgId = :songId"
        );
        sql.bind(":comment",text.toStdString());
        sql.bind(":songId",songId);
        sql.exec();
    }catch(const SQLite::Exception& e){
        auto logger{spdlog::get(LogUiID)};
        logger->error("id号{}:评论更新失败",songId);
    }
}

SongsManage::~SongsManage(){

}

juce::WebBrowserComponent::Options songsManageBuilder::buildOptions(const juce::WebBrowserComponent::Options& initial){
    return initial
    .withNativeFunction(//得到总歌曲数目
        B_getAllSongCount::name,//得到总歌曲数目
        [](const auto& args,auto complete){
            dbManager::getInstance().runOnRead([complete = std::move(complete)]{
                auto count{dbManager::getInstance().getSongsManager().getTotalSongCount()};
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
            });
        }
    ).withNativeFunction(B_toggleMyLike::name,//将我喜欢的歌曲状态翻转
        [](
            const juce::Array<juce::var>& args,
            juce::WebBrowserComponent::NativeFunctionCompletion complete
        ){
            dbManager::getInstance().runOnWrite([args,complete = std::move(complete)]{
                //const不能执行移动操作
                int64_t id{0};
                if(args.size()>=1) id = args[0];
                if(dbManager::getInstance().getSongsManager().reverseMyLike(id)){
                    complete(juce::var());
                    return;
                }else{
                    auto error{new juce::DynamicObject()};
                    error->setProperty(B_event::fullError,utf8("[我喜欢]状态更新失败，请重试"));
                    complete(juce::var(error));
                    return;
                }
            });
        }
    ).withNativeFunction(//得到单页的歌曲信息
        B_refreshAllMusicSongs::name,//参数：当前页码,升降序，排序方法，
        [](
            const juce::Array<juce::var>& args,auto complete
        ){
            dbManager::getInstance().runOnRead([args,complete = std::move(complete)]{
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
                auto results{dbManager::getInstance().getSongsManager().getSongPage(
                    targetPage,
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
            });
        }
    ).withNativeFunction(B_saveComment::name,
        [](const juce::Array<juce::var>& args,auto complete){
            juce::String text{args[0][B_saveComment::text]};
            int64_t songId{args[0][B_saveComment::songId]};
            dbManager::getInstance().runOnWrite([text,songId,complete = std::move(complete)]{
                dbManager::getInstance().getSongsManager().saveComment(
                    text,
                    songId
                );
            });
    });
}
