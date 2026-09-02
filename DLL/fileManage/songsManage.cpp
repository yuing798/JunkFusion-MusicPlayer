#include "songsManage.hpp"
#include "../dllManager.hpp"
#include "WaveFormAnaly.hpp"
#include "constants.h"
#include "dbModel.hpp"
#include "dllUtils.hpp"
#include "juce_core/juce_core.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

SongsManage::SongsManage(SQLite::Database& d) : db(d), mWaveFormAnaly(d) {
    try {
        db.exec(createSongsTableSQL);
    } catch (const std::exception& e) {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogDllID);
        if (logger)
            logger->critical(
                "无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}",
                e.what()
            );
    }
    mWaveFormAnaly.onTimeDomainSpecInsertOver = [this](const char* fileName) {
        if (onTimeDomainSpecInsertOver) onTimeDomainSpecInsertOver(fileName);
    };
}

InsertSongInfo SongsManage::insertSong(const juce::File& path) {
    SongInfo info{};
    OnlineGetMatedata::Task onlineTask{};
    std::string filePath = path.getFullPathName().toStdString();
    int64_t fileSize = path.getSize();
    std::string lastModifiedTime =
        path.getLastModificationTime().toString(true, true).toStdString();

    auto logger{spdlog::get(LogDllID)};

    // ffmpeg解码层信息
    int result{0}; // 解码层结果，一般成功返回零
    AVFormatContext* inputContext{nullptr};
    result = avformat_open_input(&inputContext, filePath.c_str(), nullptr, nullptr);
    if (result != 0) {
        // 非多媒体文件也会返回AVERROR
        // SPDLOG:记录多媒体文件无法打开文件或者打开的是非多媒体文件
        if (logger)
            logger->warn(
                "多媒体文件无法打开或者打开的是非多媒体文件:{}",
                Utils ::ffmpegErrorOutput(result)
            );
        avformat_close_input(&inputContext);
        InsertSongInfo errorInfo;
        errorInfo.errorMsg = "多媒体文件无法打开";
        return errorInfo;
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if (result < 0) {
        // SPDLOG:无法找到流信息
        if (logger) logger->error("无法找到该文件的流信息:{}", Utils::ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        InsertSongInfo errorInfo;
        errorInfo.errorMsg = "无法找到该文件的流信息";
        return errorInfo;
    }

    // 时长（秒）
    if (inputContext->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
    }

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];
    auto* decoderPar = pAudioStream->codecpar;

    if (currentIndex < 0) {
        avformat_close_input(&inputContext);
        InsertSongInfo errorInfo;
        errorInfo.errorMsg = "无法找到该文件的音频流";
        return errorInfo;
    }

    info.codecName = avcodec_get_name(decoderPar->codec_id);

    // 比特率（kbps）——先取编码器报告值，缺失时用文件大小估算
    info.bitRate = decoderPar->bit_rate / 1000;
    if (info.bitRate <= 0 && info.duration > 0.0 && fileSize > 0) {
        info.bitRate = static_cast<int>(fileSize * 8.0 / info.duration / 1000.0);
    }

    // 采样率（Hz）
    info.sampleRate = decoderPar->sample_rate;

    // 通道数
    uint64_t channelLayoutMask{0}; // 通道布局掩码
    int numChannels{0};            // 这两个先判断掩码是否有，没有再改用通道数
    if (decoderPar->ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
        channelLayoutMask = decoderPar->ch_layout.u.mask;
    }
    if (channelLayoutMask == 0) { // 掩码为0说明是不正常文件
        numChannels = decoderPar->ch_layout.nb_channels;
        info.channelLayout = juce::String(numChannels) + "声道";
    } else {
        AVChannelLayout layout;
        av_channel_layout_from_mask(&layout, channelLayoutMask);
        char layoutBuffer[64] = {0};
        av_channel_layout_describe(&layout, layoutBuffer, sizeof(layoutBuffer));

        info.channelLayout = juce::String(layoutBuffer);
    }

    // 位深 —— 仅 PCM 编码有意义，压缩编码 bits_per_coded_sample 为其解码位深
    int bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(decoderPar->format));
    if (bytesPerSample > 0) {
        info.bitDepth = bytesPerSample * 8;
    } else {
        info.bitDepth = decoderPar->bits_per_coded_sample;
    }
    // 提取文件层面的标签数据
    AVDictionary* pTags = inputContext->metadata;
    AVDictionaryEntry* pEntry = nullptr;

    if ((pEntry = av_dict_get(pTags, "title", nullptr, 0))) {
        info.title = pEntry->value;
        onlineTask.title = info.title;
    } else {
        onlineTask.title = std::nullopt;
        info.title = juce::File(filePath).getFileNameWithoutExtension().toStdString();
    }
    auto safeToInt = [](const char* str) -> int {
        try {
            return std::stoi(str);
        } catch (...) {
            return 0;
        }
    };

    if ((pEntry = av_dict_get(pTags, "artist", nullptr, 0))) {
        info.artist = pEntry->value;
        onlineTask.artist = info.artist;
    } else {
        onlineTask.artist = std::nullopt;
    }
    if ((pEntry = av_dict_get(pTags, "album", nullptr, 0))) {
        info.album = pEntry->value;
    } else {
        onlineTask.album = std::nullopt;
    }
    if ((pEntry = av_dict_get(pTags, "album_artist", nullptr, 0))) info.albumArtist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "genre", nullptr, 0))) info.genre = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "track", nullptr, 0))) {
        info.trackNumber = safeToInt(pEntry->value);
    }
    // } else {
    //     onlineTask.needTrackNumber = true;
    // }
    if ((pEntry = av_dict_get(pTags, "disc", nullptr, 0))) {
        info.discNumber = safeToInt(pEntry->value);
    }
    if ((pEntry = av_dict_get(pTags, "date", nullptr, 0))) info.year = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "composer", nullptr, 0))) info.composer = pEntry->value;

    // 这里进行封面提取
    AVPacket coverPacket;
    coverPacket.data = nullptr;
    coverPacket.size = 0;
    SHA1 sha1;
    for (size_t i = 0; i < inputContext->nb_streams; i++) {
        auto* stream{inputContext->streams[i]};
        auto type{stream->codecpar->codec_type};
        if (type == AVMEDIA_TYPE_ATTACHMENT ||
            (type == AVMEDIA_TYPE_VIDEO && (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))) {
            coverPacket = stream->attached_pic;
            break;
        }

    } // 这里只是做出了图片编码，但是没有解码，不能获得宽度和高度

    if (coverPacket.data && coverPacket.size > 0) {
        do {
            info.hash = sha1(coverPacket.data, coverPacket.size);

            juce::File hashImageDir{
                dllManager::getInstance().getSongImageDir().getChildFile(info.hash.value())
            };
            // data存在说明一定有图片，所以直接使用.value()就行了
            //  直接用哈希值作为文件夹名，所有该图片相关的缓存文件都放在同一个文件夹中

            if (hashImageDir.exists()) {
                break;
            } else {
                hashImageDir.createDirectory();
            } // 如果这个目录已经存在，直接退出，避免保存两个相同图片

            juce::File originalFile{hashImageDir.getChildFile("original.jpg")};

            juce::FileOutputStream outputStream(originalFile);
            if (outputStream.openedOk()) {
                outputStream.write(coverPacket.data, coverPacket.size);
                outputStream.flush();
            }

        } while (0);
    } else {
        onlineTask.needCover = true;
    }

    avformat_close_input(&inputContext);

    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&](SQLite::Statement& stmt) {
        // ── 可选 string 字段：有值则绑定，无值则绑定 NULL ──
        auto bindOptStr = [&](const char* name, const std::optional<std::string>& v) {
            if (v.has_value())
                stmt.bind(name, v.value());
            else
                stmt.bind(name); // 无第二个参数 → SQL NULL
        };
        // ── 可选 int 字段 ──
        auto bindOptInt = [&](const char* name, const std::optional<int>& v) {
            if (v.has_value())
                stmt.bind(name, v.value());
            else
                stmt.bind(name);
        };

        // ── 必选字段 ──
        stmt.bind(":filePath", filePath);
        stmt.bind(":fileSize", fileSize);
        stmt.bind(":lastModifiedTime", lastModifiedTime);
        stmt.bind(":duration", info.duration);
        stmt.bind(":title", info.title);
        // ── FFmpeg 必选 int 字段 ──
        stmt.bind(":bitRate", info.bitRate);
        stmt.bind(":bitDepth", info.bitDepth);
        stmt.bind(":sampleRate", info.sampleRate);
        if (channelLayoutMask != 0) {
            stmt.bind(":channelLayoutMask", static_cast<int64_t>(channelLayoutMask));
        } else {
            stmt.bind(":numChannels", numChannels);
        }

        bindOptStr(":artist", info.artist);
        bindOptStr(":album", info.album);
        bindOptStr(":albumArtist", info.albumArtist);
        bindOptStr(":genre", info.genre);
        bindOptInt(":trackNumber", info.trackNumber);
        bindOptInt(":discNumber", info.discNumber);
        bindOptInt(":year", info.year);
        bindOptStr(":composer", info.composer);
        bindOptStr(":codecName", info.codecName);
        bindOptStr(":hash", info.hash);
    };

    try {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(
                db,
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = :filePath"
            );
            checkQuery.bind(":filePath", filePath);

            if (checkQuery.executeStep()) {
                existingId = checkQuery.getColumn("songId").getInt64();
                int64_t existingSize = checkQuery.getColumn("fileSize").getInt64();
                std::string existingTime = checkQuery.getColumn("lastModifiedTime").getString();

                // 文件大小和最后修改时间完全相同 → 视为同一文件，跳过插入
                if (fileSize == existingSize && lastModifiedTime == existingTime) {
                    InsertSongInfo errorInfo;
                    errorInfo.errorMsg = "该文件已经存在";
                    return errorInfo;
                }
            }
        }

        // ── 事务开始（RAII(资源获取即初始化)：析构时若未 commit 则自动回滚，阻止插入） ──
        SQLite::Transaction transaction(db);

        if (existingId >= 0) {
            // ── 文件已变更：更新 songs 记录 ──
            SQLite::Statement updateSong(
                db,
                "UPDATE songs SET filePath = :filePath, fileSize = :fileSize, "
                "lastModifiedTime = :lastModifiedTime, "
                "duration = :duration, title = :title, artist = :artist, album = :album, "
                "albumArtist = :albumArtist, "
                "genre = :genre, trackNumber = :trackNumber, discNumber = :discNumber, year = "
                ":year, composer = :composer, "
                "bitRate = :bitRate, bitDepth = :bitDepth, hash = :hash, "
                "sampleRate = :sampleRate, channelLayoutMask = :channelLayoutMask, codecName = "
                ":codecName, numChannels = :numChannels "
                "WHERE songId = :songId"
            );

            bindSongFields(updateSong);
            updateSong.bind(":songId", existingId);
            updateSong.exec();

            // ── 回写主键：UPDATE 后 info.songId 仍是 0，必须手动赋值 ──
            info.songId = existingId;
            onlineTask.songId = existingId;
        } else {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(
                db,
                "INSERT INTO songs (filePath, fileSize, lastModifiedTime, "
                "duration, title, artist, album, albumArtist, "
                "genre, trackNumber, discNumber, year, composer, bitRate, bitDepth, "
                "sampleRate, channelLayoutMask, numChannels, codecName, hash, timeDomainSpec) "
                "VALUES (:filePath, :fileSize, :lastModifiedTime, :duration, :title, :artist, "
                ":album, :albumArtist, "
                ":genre, :trackNumber, :discNumber, :year, :composer, :bitRate, :bitDepth, "
                ":sampleRate, :channelLayoutMask, :numChannels, :codecName, :hash, :timeDomainSpec)"
            );

            bindSongFields(insertSong);
            insertSong.exec();

            // ── INSERT 后 SQLite 自动生成主键，必须读回否则 songId 始终 = 0 ──
            info.songId = db.getLastInsertRowid();
            onlineTask.songId = info.songId;
        }

        // ── 全部成功，提交事务 ──
        transaction.commit();
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("insert song error:{}", e.what());
        InsertSongInfo errorInfo;
        errorInfo.errorMsg = "数据库错误";
        return errorInfo;
    }

    InsertSongInfo insertSongInfo;
    insertSongInfo.info = info;

    WaveFormAnaly::Task waveFormAnalyTask{};
    waveFormAnalyTask.onlineTask = onlineTask;
    waveFormAnalyTask.path = filePath;

    mWaveFormAnaly.setTask(waveFormAnalyTask);

    return insertSongInfo;
}

std::vector<SongInfo> SongsManage::getAllSongs() {
    std::vector<SongInfo> result;

    try {
        std::string sql = R"(
            SELECT songId, 
            duration, title, artist, album, albumArtist, 
            genre, trackNumber, discNumber, year, composer, 
            bitRate, bitDepth, sampleRate, channelLayoutMask, numChannels, codecName, 
            aiGenre, bpm, key, aiProcessed, 
            isMyLike, comment, playNum, hash
            FROM songs 
        )"; // 按照顺序查询的sql语句

        SQLite::Statement query(db, sql);

        // ── 辅助：读取可能为 NULL 的 string 列 → std::optional<std::string> ──
        auto optStrCol = [&](const char* colName) -> std::optional<std::string> {
            auto col = query.getColumn(colName);
            if (col.isNull()) return std::nullopt;
            std::string s = col.getString();
            if (s.empty()) {
                return std::nullopt;
            } else {
                return s;
            }
        };
        // ── 辅助：读取可能为 NULL 的 int 列 → std::optional<int> ──
        auto optIntCol = [&](const char* colName) -> std::optional<int> {
            if (query.getColumn(colName).isNull()) return std::nullopt;
            return query.getColumn(colName).getInt();
        };

        while (query.executeStep()) {
            SongInfo info{};
            info.songId = query.getColumn("songId").getInt64();
            info.duration = query.getColumn("duration").getDouble();

            // ── 标签 ──
            info.title = query.getColumn("title").getString();
            info.artist = optStrCol("artist");
            info.album = optStrCol("album");
            info.albumArtist = optStrCol("albumArtist");
            info.genre = optStrCol("genre");
            info.trackNumber = optIntCol("trackNumber");
            info.discNumber = optIntCol("discNumber");
            info.year = optIntCol("year");
            info.composer = optStrCol("composer");

            // ── FFmpeg 解码层 ──
            info.bitRate = query.getColumn("bitRate").getInt64();
            info.bitDepth = query.getColumn("bitDepth").getInt();
            info.sampleRate = query.getColumn("sampleRate").getInt();

            auto channelLayoutMask = query.getColumn("channelLayoutMask").getInt64();
            int numChannels = query.getColumn("numChannels").getInt();
            if (channelLayoutMask == 0) {
                info.channelLayout = juce::String(numChannels) + "声道";
            } else {
                AVChannelLayout layout;
                av_channel_layout_from_mask(&layout, channelLayoutMask);
                char layoutBuffer[64] = {0};
                av_channel_layout_describe(&layout, layoutBuffer, sizeof(layoutBuffer));
                info.channelLayout = juce::String(layoutBuffer);
            }
            info.codecName = optStrCol("codecName");

            // ── AI 分析 ──
            info.aiGenre = optStrCol("aiGenre");
            info.bpm = optIntCol("bpm");
            info.key = optStrCol("key");

            // ── 用户信息 ──
            info.isMyLike = query.getColumn("isMyLike").getInt() != 0;
            info.comment = optStrCol("comment");
            info.playNum = query.getColumn("playNum").getInt();
            info.hash = optStrCol("hash");

            result.push_back(std::move(info));
        }
    } catch (const SQLite::Exception& e) {
        auto logger = spdlog::get(LogDllID);
        logger->error("getAllSongs发生失败:{}", e.what());
    }

    return result;
}

std::string SongsManage::getPathBySongId(int64_t songId) {
    spdlog::get(LogDllID)->debug("开始根据id搜索歌曲路径");
    std::string path;
    try {
        SQLite::Statement sql(
            db,
            R"(
            SELECT filePath, duration
            FROM songs WHERE songId = :songId)"
        );
        sql.bind(":songId", songId);
        if (sql.executeStep()) {
            path = sql.getColumn("filePath").getString();
        } else {
            throw SQLite::Exception{"我草他妈的找不着:" + std::to_string(songId)};
        }
        spdlog::get(LogDllID)->debug("成功获取:id:{},路径:{}", songId, path);
        return path;
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogDllID)};
        logger->error("SongsManage::getPlayInfoBySongId(int64_t songId)发生错误:{}", e.what());
        return "";
    }
}

bool SongsManage::reverseMyLike(int64_t id) {

    try {
        SQLite::Statement sql(
            db,
            "UPDATE songs SET isMyLike = 1 - isMyLike WHERE songId = :songId"
        );
        sql.bind(":songId", id);

        if (sql.exec() > 0) {
            return true;
        } else {
            return false;
        }
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogDllID)};
        logger->error("[我喜欢]状态更新失败，请重试:{}", e.what());
        return false;
    }
}

void SongsManage::saveComment(juce::String text, int64_t songId) {
    try {
        SQLite::Statement sql(db, "UPDATE songs SET comment = :comment WHERE songId = :songId");
        sql.bind(":comment", text.toStdString());
        sql.bind(":songId", songId);
        sql.exec();
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogDllID)};
        logger->error("id号{}:评论更新失败:{}", songId, e.what());
    }
}

juce::Array<double> SongsManage::getTimeDomainSpecBySongId(int64_t songId) {
    try {
        SQLite::Statement sql(db, "SELECT timeDomainSpec FROM songs WHERE songId = :songId");
        sql.bind(":songId", songId);
        if (sql.executeStep()) {
            const void* blob{sql.getColumn("timeDomainSpec").getBlob()};
            int size{sql.getColumn("timeDomainSpec").getBytes()}; // 字节数
            juce::Array<double> buffer;
            if (128 != (size / sizeof(double))) {
                throw SQLite::Exception("波形图发生损坏");
            }
            buffer.resize(size / sizeof(double));
            std::memcpy(buffer.data(), blob, size);
            return buffer;
        } else {
            throw SQLite::Exception("无法执行时域图读取");
        }
    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)
            ->error("getTimeDomainSpecBySongId错误:id:{},原因:{}", songId, e.what());
        return {};
    }
}

// SongsManage::~SongsManage() {}
