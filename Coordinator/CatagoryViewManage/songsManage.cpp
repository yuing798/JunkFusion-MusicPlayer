#include "./songsManage.hpp"
#include "../dllManager.hpp"
#include "Model/PlayInfo.hpp"
#include "Model/SongInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <cstring>

#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <sha1.h>
#include <spdlog/spdlog.h>
#include <string>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

std::vector<std::string> SongsManage::getTags(TagLib::PropertyMap& map, const char* key) {
    if (!map.contains(key) || map[key].isEmpty()) {
        return std::vector<std::string>{};
    }

    const TagLib::StringList& rawList = map[key];
    std::vector<std::string> finalItems;

    // 如果 TagLib 提取出了真正的多元素数组 (FLAC 或 ID3v2.4)
    if (rawList.size() > 1) {
        for (const auto& item : rawList) {
            finalItems.push_back(item.to8Bit(true));
        }
    }
    // 如果 TagLib 只提取到了 1 个元素 (旧版 ID3v2.3 硬编码斜杠的情况)
    else {
        std::string rawStr = rawList.front().to8Bit(true);
        std::string tagKey = key;

        // 转为大写，防止调用者传入 "artist" 或 "genre" 导致匹配失败
        std::transform(tagKey.begin(), tagKey.end(), tagKey.begin(), [](unsigned char c) {
            return std::toupper(c);
        });

        // 仅针对 ARTIST 和 GENRE 进行斜杠/分号的二次拆分兼容
        if ((tagKey == "ARTIST" || tagKey == "GENRE") && rawStr != "AC/DC" && // 白名单特例规避
            (rawStr.find('/') != std::string::npos || rawStr.find(';') != std::string::npos)) {
            std::string token;
            for (char ch : rawStr) {
                if (ch == '/' || ch == ';') {
                    // 去除前后空格并保存
                    auto start = token.find_first_not_of(" ");
                    auto end = token.find_last_not_of(" ");
                    if (start != std::string::npos) {
                        finalItems.push_back(token.substr(start, end - start + 1));
                    }
                    token.clear();
                } else {
                    token += ch;
                }
            }
            // 压入最后一个名字
            auto start = token.find_first_not_of(" ");
            auto end = token.find_last_not_of(" ");
            if (start != std::string::npos) {
                finalItems.push_back(token.substr(start, end - start + 1));
            }
        } else {
            finalItems.push_back(rawStr);
        }
    }

    return finalItems;
}

std::optional<int> SongsManage::getTagInt(TagLib::PropertyMap& map, const char* key) {
    // auto tags{getTags(map, key)};
    if (!map.contains(key) || map[key].isEmpty()) {
        return std::nullopt;
    }
    auto& rawList{map[key]};

    try {
        if (rawList.isEmpty()) {
            return std::nullopt;
        }
        return std::stoi(rawList[0].to8Bit());
    } // std::stoi 解析 "2023-05-12" 会自动提取 2023
    catch (...) {
        return std::nullopt;
    }
}

SongInfo SongsManage::getBasicInfoCol(SQLite::Statement& query) {
    SongInfo info{};

    info.songId = query.getColumn("songId").getInt64();
    info.duration = query.getColumn("duration").getDouble();

    // ── 标签 ──
    info.title = query.getColumn("title").getString();
    info.artists = DbUtils::getOptCol<std::vector<std::string>>(query, "artists");
    info.album = DbUtils::getOptCol<std::string>(query, "album");

    // ── 用户信息 ──
    info.isMyLike = query.getColumn("isMyLike").getInt() != 0;
    info.playNum = query.getColumn("playNum").getInt();
    info.hash = DbUtils::getOptCol<std::string>(query, "hash");
    return info;
}
SongInfo SongsManage::getDetailInfoCol(SQLite::Statement& query) {
    SongInfo info{};
    info.trackNumber = DbUtils::getOptCol<int>(query, "trackNumber");
    info.discNumber = DbUtils::getOptCol<int>(query, "discNumber");
    // ── FFmpeg 解码层 ──

    info.bitRate = query.getColumn("bitRate").getInt64();
    info.bitDepth = query.getColumn("bitDepth").getInt();
    info.sampleRate = query.getColumn("sampleRate").getInt();

    auto channelLayoutMask = query.getColumn("channelLayoutMask").getInt64();
    if (channelLayoutMask != 0) {

        AVChannelLayout layout;
        av_channel_layout_from_mask(&layout, channelLayoutMask);
        char layoutBuffer[64] = {0};
        av_channel_layout_describe(&layout, layoutBuffer, sizeof(layoutBuffer));
        info.channelLayout = std::string(layoutBuffer);
    }
    info.codecName = DbUtils::getOptCol<std::string>(query, "codecName");

    // ── AI 分析 ──
    info.bpm = DbUtils::getOptCol<int>(query, "bpm");
    info.key = DbUtils::getOptCol<std::string>(query, "key");

    info.timeDomainSpec = DbUtils::getBlob<double>(query, "timeDomainSpec");
    return info;
}
SongInfo SongsManage::getBasicSongInfo(int64_t songId) {
    try {
        SQLite::Statement query{
            dbManage.readDb(),
            R"(
                SELECT songId, 
                duration, title, artists, album, 
                isMyLike, playNum, hash
                FROM songs WHERE songId = :songId
            )"
        };
        query.bind(":songId", songId);

        SongInfo info;
        if (query.executeStep()) {
            info = getBasicInfoCol(query);
        } else {
            throw SQLite::Exception{"无法根据id找到歌曲信息:路径:" + std::to_string(songId)};
        }
        return info;

    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->info(
            "SongsManage::getBasicSongInfo(int64_t songId)发生数据库错误:{}",
            e.what()
        );
        return {};
    }
}

SongInfo SongsManage::getDetailSongInfo(int64_t songId) {
    try {
        SQLite::Statement query{
            dbManage.readDb(),
            R"(
                SELECT 
                trackNumber,discNumber,
                bitRate, bitDepth, sampleRate, channelLayoutMask,  codecName,
                bpm, key, 
                timeDomainSpec 
                FROM songs WHERE songId = :songId
            )"
        };
        query.bind(":songId", songId);

        SongInfo info;
        if (query.executeStep()) {
            info = getDetailInfoCol(query);
        } else {
            throw SQLite::Exception{"无法根据id找到歌曲信息:路径:" + std::to_string(songId)};
        }
        return info;

    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->info(
            "SongsManage::getDetailSongInfo(int64_t songId)发生数据库错误:{}",
            e.what()
        );
        return {};
    }
}
nlohmann::json SongsManage::getAllSongsBasicInfo() {
    nlohmann::json results;

    try {

        SQLite::Statement query(dbManage.readDb(), R"(
            SELECT songId, 
            duration, title, artists, album, 
            isMyLike, playNum, hash
            FROM songs ORDER BY songId
        )");

        while (query.executeStep()) {

            results.push_back(getBasicInfoCol(query).toBasicJson());
        }
    } catch (const SQLite::Exception& e) {
        auto logger = spdlog::get(LogDllID);
        logger->error("getAllSongsBasicInfo发生失败:{}", e.what());
    }

    return results;
}
std::variant<std::string, SongsManage::InsertSongResult>
SongsManage::insertSong(const juce::File& path) {
    SongInfo info{};
    InsertSongResult result;
    av_log_set_level(AV_LOG_ERROR);
    std::string filePath = path.getFullPathName().toStdString();
    int64_t fileSize = path.getSize();
    std::string lastModifiedTime =
        path.getLastModificationTime().toString(true, true).toStdString();

    auto logger{spdlog::get(LogDllID)};

    // ffmpeg decoder info
    AVFormatContext* inputContext{nullptr};
    {
        int avResult = avformat_open_input(&inputContext, filePath.c_str(), nullptr, nullptr);
        if (avResult != 0) {
            // 非多媒体文件也会返回AVERROR
            if (logger)
                logger->warn(
                    "多媒体文件无法打开或者打开的是非多媒体文件:{}",
                    OtherUtils ::ffmpegErrorOutput(avResult)
                );
            avformat_close_input(&inputContext);

            return std::string{"无法打开多媒体文件"};
        }
    }
    {
        int avResult = avformat_find_stream_info(inputContext, nullptr);
        if (avResult < 0) {
            if (logger)
                logger->error("无法找到该文件的流信息:{}", OtherUtils::ffmpegErrorOutput(avResult));
            avformat_close_input(&inputContext);

            return std::string{"无法找到该文件的流信息"};
        }
    }
    // length(seconds)
    if (inputContext->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
    }

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];
    auto* decoderPar = pAudioStream->codecpar;

    if (currentIndex < 0) {
        avformat_close_input(&inputContext);

        return std::string{"无法找到该文件的音频流"};
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
    if (decoderPar->ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
        channelLayoutMask = decoderPar->ch_layout.u.mask;
    }
    if (channelLayoutMask != 0) {
        AVChannelLayout layout;
        av_channel_layout_from_mask(&layout, channelLayoutMask);
        char layoutBuffer[64] = {0};
        av_channel_layout_describe(&layout, layoutBuffer, sizeof(layoutBuffer));

        info.channelLayout = std::string(layoutBuffer);
    }

    // 位深 —— 仅 PCM 编码有意义，压缩编码 bits_per_coded_sample 为其解码位深
    int bytesPerSample = av_get_bytes_per_sample(static_cast<AVSampleFormat>(decoderPar->format));
    if (bytesPerSample > 0) {
        info.bitDepth = bytesPerSample * 8;
    } else {
        info.bitDepth = decoderPar->bits_per_coded_sample;
    }

    // 这里进行tagLib的元数据提取
    {
#if _WIN32
        // Windows 下 TagLib 需要传入宽字符 (wchar_t) 才能支持中文路径
        TagLib::FileRef f(path.getFullPathName().toWideCharPointer());
#else
        // macOS / Linux 下直接传 UTF-8 字符串即可
        TagLib::FileRef f(path.getFullPathName().toUTF8());
#endif

        // 容错处理：如果标题为空，默认使用去后缀的文件名
        auto defaultTitle = path.getFileNameWithoutExtension().toStdString();

        if (!f.isNull() && f.file()) {
            // 获取统一属性映射表 (PropertyMap)，这是抹平所有格式差异的核心！
            TagLib::PropertyMap map = f.file()->properties();

            // Title
            auto titles = getTags(map, "TITLE");
            if (!titles.empty()) {
                info.title = titles[0];
                result.titleIsFileName = false;
            } else {
                info.title = defaultTitle;
                result.titleIsFileName = true;
            }

            // Artist
            auto artists = getTags(map, "ARTIST");
            if (!artists.empty()) {
                info.artists = artists;
            }

            // Album
            auto albums = getTags(map, "ALBUM");
            if (!albums.empty()) {
                info.album = albums[0];
            }

            {
                auto albumArtists{getTags(map, "ALBUMARTIST")};
                if (!albumArtists.empty()) {
                    result.albumArtists = albumArtists;
                }
            }

            // 数值型信息
            info.trackNumber = getTagInt(map, "TRACKNUMBER");
            info.discNumber = getTagInt(map, "DISCNUMBER");

        } else {
            info.title = defaultTitle;
            result.titleIsFileName = true;
        }
    }

    // 这里进行封面提取
    {
        AVPacket coverPacket;
        coverPacket.data = nullptr;
        coverPacket.size = 0;
        SHA1 sha1;
        for (size_t i = 0; i < inputContext->nb_streams; i++) {
            auto* stream{inputContext->streams[i]};
            auto type{stream->codecpar->codec_type};
            if (type == AVMEDIA_TYPE_ATTACHMENT ||
                (type == AVMEDIA_TYPE_VIDEO &&
                 (stream->disposition & AV_DISPOSITION_ATTACHED_PIC))) {
                coverPacket = stream->attached_pic;
                break;
            }

        } // 这里只是做出了图片编码，但是没有解码，不能获得宽度和高度

        if (coverPacket.data && coverPacket.size > 0) {
            do {
                info.hash = sha1(coverPacket.data, coverPacket.size);
                if (!info.hash.has_value()) break;

                juce::File hashImageDir{
                    dllManager::getInstance().getImageDir().getChildFile(info.hash.value())
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
        }
    }

    avformat_close_input(&inputContext);

    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&](SQLite::Statement& stmt) {
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
        }
        DbUtils::bindOptValue(stmt, ":artists", info.artists);

        DbUtils::bindOptValue(stmt, ":album", info.album);
        DbUtils::bindOptValue(stmt, ":trackNumber", info.trackNumber);
        DbUtils::bindOptValue(stmt, ":discNumber", info.discNumber);
        DbUtils::bindOptValue(stmt, ":codecName", info.codecName);
        DbUtils::bindOptValue(stmt, ":hash", info.hash);
    };

    try {
        // ── 第 1 道防线：代码层查询 file_path，比较 size 和 last_modified_time ──
        int64_t existingId = -1;

        {
            SQLite::Statement checkQuery(
                dbManage.readDb(),
                "SELECT songId, fileSize, lastModifiedTime FROM songs WHERE filePath = "
                ":filePath"
            );
            checkQuery.bind(":filePath", filePath);

            if (checkQuery.executeStep()) {
                existingId = checkQuery.getColumn("songId").getInt64();
                int64_t existingSize = checkQuery.getColumn("fileSize").getInt64();
                std::string existingTime = checkQuery.getColumn("lastModifiedTime").getString();

                // 文件大小和最后修改时间完全相同 → 视为同一文件，跳过插入
                if (fileSize == existingSize && lastModifiedTime == existingTime) {

                    return std::string{"该文件已存在"};
                }
            }
        }

        dbManage.write([&](SQLite::Database& db) {
            if (existingId >= 0) {
                // ── 文件已变更：更新 songs 记录 ──
                SQLite::Statement updateSong(
                    db,
                    "UPDATE songs SET filePath = :filePath, fileSize = :fileSize, "
                    "lastModifiedTime = :lastModifiedTime, "
                    "duration = :duration, title = :title, artists = :artists, album = :album, "
                    "trackNumber = :trackNumber,  discNumber = :discNumber,"
                    "bitRate = :bitRate, bitDepth = :bitDepth, hash = :hash, "
                    "sampleRate = :sampleRate, channelLayoutMask = :channelLayoutMask, "
                    "codecName = :codecName "
                    "WHERE songId = :songId"
                );

                bindSongFields(updateSong);
                updateSong.bind(":songId", existingId);
                updateSong.exec();

                // ── 回写主键：UPDATE 后 info.songId 仍是 0，必须手动赋值 ──
                info.songId = existingId;
            } else {
                // ── 新文件：插入 songs 记录 ──
                SQLite::Statement insert(
                    db,
                    "INSERT INTO songs (filePath, fileSize, lastModifiedTime, "
                    "duration, title, artists, album, "
                    "trackNumber, discNumber,bitRate, bitDepth, "
                    "sampleRate, channelLayoutMask, codecName, hash) "

                    " VALUES (:filePath, :fileSize, :lastModifiedTime, :duration, :title, "
                    ":artists, "
                    ":album, "
                    ":trackNumber, :discNumber,:bitRate, :bitDepth, "
                    ":sampleRate, :channelLayoutMask, :codecName, :hash) "
                );

                bindSongFields(insert);
                insert.exec();

                // ── INSERT 后 SQLite 自动生成主键，必须读回否则 songId 始终 = 0 ──
                info.songId = db.getLastInsertRowid();
            }
        });
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("insert song error:{}", e.what());

        return std::string{"数据库错误"};
    }

    result.info = std::move(info);

    return result;
}

PlayInfo SongsManage::getPlayInfo(int64_t songId) {
    spdlog::get(LogDllID)->debug("开始根据id搜索歌曲信息");
    PlayInfo info{};
    try {
        SQLite::Statement sql(
            dbManage.readDb(),
            R"(
            SELECT filePath, duration, title, artists 
            FROM songs WHERE songId = :songId)"
        );
        sql.bind(":songId", songId);
        if (sql.executeStep()) {
            info.path = sql.getColumn("filePath").getString();
            info.duration = sql.getColumn("duration").getDouble();
            info.title = sql.getColumn("title").getString();
            Yvar originArtists{nlohmann::json::parse(sql.getColumn("artists").getString())};
            info.artists = originArtists.toStringArray();

        } else {
            throw SQLite::Exception{"我草他妈的找不着:" + std::to_string(songId)};
        }
        spdlog::get(LogDllID)->debug("成功获取:id:{},路径:{}", songId, info.path);
        return info;
    } catch (const SQLite::Exception& e) {
        auto logger{spdlog::get(LogDllID)};
        logger->error("SongsManage::getPlayInfoBySongId(int64_t songId)发生错误:{}", e.what());
        return {};
    }
}

void SongsManage::reverseMyLike(int64_t id) {

    dbManage.write([id](SQLite::Database& db) {
        try {
            SQLite::Statement sql(
                db,
                "UPDATE songs SET isMyLike = 1 - isMyLike WHERE songId = :songId"
            );
            sql.bind(":songId", id);

            sql.exec();
        } catch (const SQLite::Exception& e) {
            auto logger{spdlog::get(LogDllID)};
            logger->error("[我喜欢]状态更新失败，请重试:{}", e.what());
        }
    });
    if (dllManager::getInstance().onUpdateSongInfo) dllManager::getInstance().onUpdateSongInfo(id);
}
void SongsManage::updatePlayCount(std::string path) {
    dbManage.write([&](SQLite::Database& db) {
        try {
            SQLite::Statement sql{
                db,
                "UPDATE songs SET playNum = (playNum + 1) WHERE filePath = :filePath RETURNING "
                "songId"
            };

            sql.bind(":filePath", path);
            if (sql.executeStep()) {
                int64_t songId{sql.getColumn("songId").getInt64()};
                if (dllManager::getInstance().onUpdateSongInfo)
                    dllManager::getInstance().onUpdateSongInfo(songId);
            }
            return true;
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug(
                "SongsManage::updatePlayCount(std::string path)发生错误:{}",
                e.what()
            );
            return false;
        }
    });
}