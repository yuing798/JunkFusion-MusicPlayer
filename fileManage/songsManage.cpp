#include "songsManage.hpp"
#include "../engineAudio/model.h"
#include "./dbManager.hpp"
#include "BridgeNames.h"
#include "constants.h"
#include "dbModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "otherUtils.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <libavutil/channel_layout.h>
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
}

SongsManage::SongsManage(SQLite::Database& d) : db(d) {
    try {
        db.exec(createSongsTableSQL);
    } catch (const std::exception& e) {
        // 数据库初始化失败 → db 保持 nullptr，后续所有操作安全返回空
        auto logger = spdlog::get(LogAllID);
        if (logger)
            logger->critical("无法初始化数据库文件，请检查磁盘空间或权限！\n错误信息: {}",
                             e.what());
    }
}

std::optional<SongInfo> SongsManage::insertSong(const juce::File& path) {
    SongInfo info{};
    std::string filePath = path.getFullPathName().toStdString();
    int64_t fileSize = path.getSize();
    std::string lastModifiedTime =
        path.getLastModificationTime().toString(true, true).toStdString();

    auto logger{spdlog::get(LogSchedulerID)};

    // ffmpeg解码层信息
    int result{0}; // 解码层结果，一般成功返回零
    AVFormatContext* inputContext{nullptr};
    result = avformat_open_input(&inputContext, filePath.c_str(), nullptr, nullptr);
    if (result != 0) {
        // 非多媒体文件也会返回AVERROR
        // SPDLOG:记录多媒体文件无法打开文件或者打开的是非多媒体文件
        if (logger)
            logger->warn("多媒体文件无法打开或者打开的是非多媒体文件:{}",
                         ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return std::nullopt;
    }
    result = avformat_find_stream_info(inputContext, nullptr);
    if (result < 0) {
        // SPDLOG:无法找到流信息
        if (logger) logger->error("无法找到该文件的流信息:{}", ffmpegErrorOutput(result));
        avformat_close_input(&inputContext);
        return std::nullopt;
    }

    // 时长（秒）
    if (inputContext->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
    } // 每条流的时长都一样

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];
    auto* decoderPar = pAudioStream->codecpar;
    int codecID = decoderPar->codec_id;
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
        char buffer[64] = {0};
        int result = av_channel_layout_describe(&layout, buffer, sizeof(buffer));
        if (result > 0) {
            info.channelLayout = juce::String(buffer);
        } else {
            numChannels = decoderPar->ch_layout.nb_channels;
            info.channelLayout = juce::String(numChannels) + "声道";
        }
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
    } else {
        info.title = juce::File(filePath).getFileNameWithoutExtension().toStdString();
    }
    auto safeToInt = [](const char* str) -> int {
        try {
            return std::stoi(str);
        } catch (...) {
            return 0;
        }
    };

    if ((pEntry = av_dict_get(pTags, "artist", nullptr, 0))) info.artist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "album", nullptr, 0))) info.album = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "album_artist", nullptr, 0))) info.albumArtist = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "genre", nullptr, 0))) info.genre = pEntry->value;
    if ((pEntry = av_dict_get(pTags, "track", nullptr, 0)))
        info.trackNumber = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "disc", nullptr, 0)))
        info.discNumber = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "date", nullptr, 0))) info.year = safeToInt(pEntry->value);
    if ((pEntry = av_dict_get(pTags, "composer", nullptr, 0))) info.composer = pEntry->value;

    // 这里进行封面提取
    AVPacket coverPacket;
    std::string hash;
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
            hash = sha1(coverPacket.data, coverPacket.size);

            juce::File hashImageDir{songImageDirId.getChildFile(hash)};
            // 直接用哈希值作为文件夹名，所有该图片相关的缓存文件都放在同一个文件夹中

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
        stmt.bind(":codecId", codecID);

        bindOptStr(":artist", info.artist);
        bindOptStr(":album", info.album);
        bindOptStr(":albumArtist", info.albumArtist);
        bindOptStr(":genre", info.genre);
        bindOptInt(":trackNumber", info.trackNumber);
        bindOptInt(":discNumber", info.discNumber);
        bindOptInt(":year", info.year);
        bindOptStr(":composer", info.composer);
        bindOptStr(":codecName", info.codecName);
        bindOptStr(":hash", hash);
    };

    try {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(
                db,
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = :filePath");
            checkQuery.bind(":filePath", filePath);

            if (checkQuery.executeStep()) {
                existingId = checkQuery.getColumn("songId").getInt64();
                int64_t existingSize = checkQuery.getColumn("fileSize").getInt64();
                std::string existingTime = checkQuery.getColumn("lastModifiedTime").getString();

                // 文件大小和最后修改时间完全相同 → 视为同一文件，跳过插入
                if (fileSize == existingSize && lastModifiedTime == existingTime) {
                    return std::nullopt;
                }
            }
        }

        // ── 事务开始（RAII(资源获取即初始化)：析构时若未 commit 则自动回滚，阻止插入） ──
        SQLite::Transaction transaction(db);

        if (existingId >= 0) {
            // ── 文件已变更：更新 songs 记录 ──
            SQLite::Statement updateSong(
                db, "UPDATE songs SET filePath = :filePath, fileSize = :fileSize, "
                    "lastModifiedTime = :lastModifiedTime, "
                    "duration = :duration, title = :title, artist = :artist, album = :album, "
                    "albumArtist = :albumArtist, "
                    "genre = :genre, trackNumber = :trackNumber, discNumber = :discNumber, year = "
                    ":year, composer = :composer, "
                    "bitRate = :bitRate, bitDepth = :bitDepth, hash = :hash, "
                    "sampleRate = :sampleRate, channelLayoutMask = :channelLayoutMask, codecName = "
                    ":codecName, numChannels = :numChannels "
                    ", codecId = :codecId"
                    "WHERE songId = :songId");

            bindSongFields(updateSong);
            updateSong.bind(":songId", existingId);
            updateSong.exec();

            // ── 回写主键：UPDATE 后 info.songId 仍是 0，必须手动赋值 ──
            info.songId = existingId;
        } else {
            // ── 新文件：插入 songs 记录 ──
            SQLite::Statement insertSong(
                db, "INSERT INTO songs (filePath, fileSize, lastModifiedTime, "
                    "duration, title, artist, album, albumArtist, codecId, "
                    "genre, trackNumber, discNumber, year, composer, bitRate, bitDepth, "
                    "sampleRate, channelLayoutMask, numChannels, codecName, hash) "
                    "VALUES (:filePath, :fileSize, :lastModifiedTime, :duration, :title, :artist, "
                    ":album, :albumArtist, :codecId, "
                    ":genre, :trackNumber, :discNumber, :year, :composer, :bitRate, :bitDepth, "
                    ":sampleRate, :channelLayoutMask, :numChannels, :codecName, :hash)");

            bindSongFields(insertSong);
            insertSong.exec();

            // ── INSERT 后 SQLite 自动生成主键，必须读回否则 songId 始终 = 0 ──
            info.songId = db.getLastInsertRowid();
        }

        // ── 全部成功，提交事务 ──
        transaction.commit();
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("insert song error:{}", e.what());
        return std::nullopt;
    }

    return info;
}

std::optional<int> SongsManage::getTotalSongCount() {
    SQLite::Statement query(db, "SELECT COUNT(*) FROM songs");
    if (query.executeStep()) // exec返回的是受影响的函数，executeStep返回是否还有需要执行的行
        return query.getColumn(0).getInt();
    return std::nullopt;
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
            isMyLike, comment, playNum
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
                char buffer[64] = {0};
                int result = av_channel_layout_describe(&layout, buffer, sizeof(buffer));
                if (result > 0) {
                    info.channelLayout = juce::String(buffer);
                } else {
                    info.channelLayout = juce::String(numChannels) + "声道";
                }
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

            result.push_back(std::move(info));
        }
    } catch (const SQLite::Exception& e) {
        auto logger = spdlog::get(LogUiID);
        logger->error("getAllSongs发生失败:{}", e.what());
    }

    return result;
}

std::optional<playInfo> SongsManage::getPlayInfoBySongId(int64_t songId) {
    playInfo info{};
    try {
        SQLite::Statement sql(db,
                              R"(
            "SELECT filePath, duration, bitRate, bitDepth,
            sampleRate, numChannels, codecId
            FROM songs WHERE songId = :songId")");
        sql.bind(":songId", songId);
        if (sql.executeStep()) {
            info.path = sql.getColumn("filePath").getString();
            info.bitDepth = sql.getColumn("bitDepth").getInt();
            info.bitRate = sql.getColumn("bitRate").getInt64();
            info.originalSampleRate = sql.getColumn("sampleRate").getDouble();
            info.originalNumChannels = sql.getColumn("numChannels").getInt();
            info.duration = sql.getColumn("duration").getDouble();
            info.codecId = sql.getColumn("codecId").getInt();
        } else {
            throw SQLite::Exception{"我草他妈的找不着:" + std::to_string(songId)};
        }
        return info;
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogAudioID)};
        logger->error("SongsManage::getPlayInfoBySongId(int64_t songId)发生错误:{}", e.what());
        return std::nullopt;
    }
}

bool SongsManage::reverseMyLike(int64_t id) {

    try {
        SQLite::Statement sql(db,
                              "UPDATE songs SET isMyLike = 1 - isMyLike WHERE songId = :songId");
        sql.bind(":songId", id);

        if (sql.exec() > 0) {
            return true;
        } else {
            return false;
        }
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogUiID)};
        logger->error("[我喜欢]状态更新失败，请重试:{}", e.what());
        return false;
    }
}

std::string SongsManage::getImageHashBySongId(int64_t id) {
    try {
        SQLite::Statement sql(db, "SELECT hash FROM songs WHERE songId = :songId");
        sql.bind(":songId", id);
        if (sql.executeStep()) {
            return sql.getColumn("hash").getString();
        }
    } catch (const SQLite::Exception& e) {
        auto logger = spdlog::get(LogUiID);
        logger->error("获取歌曲封面哈希值失败:{}", e.what());
    }
    return "";
}

void SongsManage::saveComment(juce::String text, int64_t songId) {
    try {
        SQLite::Statement sql(db, "UPDATE songs SET comment = :comment WHERE songId = :songId");
        sql.bind(":comment", text.toStdString());
        sql.bind(":songId", songId);
        sql.exec();
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogUiID)};
        logger->error("id号{}:评论更新失败:{}", songId, e.what());
    }
}

SongsManage::~SongsManage() {}

juce::WebBrowserComponent::Options
songsManageBuilder::buildOptions(const juce::WebBrowserComponent::Options& initial) {
    return initial
        .withNativeFunction(         // 得到总歌曲数目
            B_getAllSongCount::name, // 得到总歌曲数目
            [](const auto& args, auto complete) {
                dbManager::getInstance().runOnRead([complete = std::move(complete)] {
                    auto count{dbManager::getInstance().getSongsManager().getTotalSongCount()};
                    auto obj{new juce::DynamicObject()};
                    if (count.has_value()) {
                        obj->setProperty(B_getAllSongCount::count, count.value());
                        complete(obj);
                        return;
                    } else {
                        obj->setProperty(B_getAllSongCount::count, 0);
                        complete(obj);
                        return;
                    }
                });
            })
        .withNativeFunction(
            B_toggleMyLike::name, // 将我喜欢的歌曲状态翻转
            [](const juce::Array<juce::var>& args,
               juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                dbManager::getInstance().runOnWrite([args, complete = std::move(complete)] {
                    // const不能执行移动操作
                    int64_t id{0};
                    if (args.size() >= 1) id = args[0];
                    if (dbManager::getInstance().getSongsManager().reverseMyLike(id)) {
                        complete(juce::var());
                        return;
                    } else {
                        auto error{new juce::DynamicObject()};
                        error->setProperty(B_event::fullError,
                                           utf8("[我喜欢]状态更新失败，请重试"));
                        complete(juce::var(error));
                        return;
                    }
                });
            })
        .withNativeFunction(     // 得到所有歌曲信息
            B_getAllSongs::name, // 参数：当前页码,升降序，排序方法，
            [](const juce::Array<juce::var>& args, auto complete) {
                dbManager::getInstance().runOnRead([complete = std::move(complete)] {
                    std::vector<SongInfo> songs =
                        dbManager::getInstance().getSongsManager().getAllSongs();
                    if (!songs.empty()) {
                        complete(SongInfo::vector2VarArray(songs));
                    } else {
                        complete(B_getAllSongs::nothing);
                    }
                });
            })
        .withNativeFunction(
            B_saveComment::name, // 保存对单首歌曲的评论
            [](const juce::Array<juce::var>& args, auto complete) {
                juce::String text{args[0][B_saveComment::text].toString()};
                int64_t songId{args[0][B_saveComment::songId]};
                dbManager::getInstance().runOnWrite([text, songId, complete = std::move(complete)] {
                    dbManager::getInstance().getSongsManager().saveComment(text, songId);
                    complete(juce::var());
                });
            });
}
