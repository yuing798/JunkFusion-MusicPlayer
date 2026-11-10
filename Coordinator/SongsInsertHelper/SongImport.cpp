#include "./SongImport.hpp"
#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Model/AlbumInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include "Utils/fileUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Statement.h>
#include <cstddef>
#include <fileref.h>
#include <optional>
#include <sha1.h>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

juce::File SongImport::getCachedFileFor(const juce::File& origin) const {
    SHA1 sha1;
    // 原文件完整路径的 SHA1 + 原扩展名。路径是确定的，所以重复导入同一份文件
    // 一定能算回同一个缓存路径，靠它判重/覆盖，不需要把原路径存进数据库。
    const std::string hash = sha1(origin.getFullPathName().toStdString());
    const std::string extension = origin.getFileExtension().toStdString();
    return mSongsDir.getChildFile(hash + extension);
}

SongHasImportRes SongImport::checkSongHasImport(const juce::File& path, bool isSingleFile) {
    try {
        // 单独导入的歌曲在库里存的是缓存路径，先把它算出来再查
        const juce::File storedFile = isSingleFile ? getCachedFileFor(path) : path;

        SQLite::Statement checkQuery(
            mDbContext.dbManage.readDb(),
            "SELECT lastModifiedTime FROM songs WHERE filePath = "
            ":filePath"
        );
        checkQuery.bind(":filePath", storedFile.getFullPathName().toStdString());

        if (checkQuery.executeStep()) {
            // 单独导入：缓存副本被人删了，就算源文件没变也得重新拷一份
            if (isSingleFile && !storedFile.existsAsFile()) {
                return SongHasImportRes::NeedUpdate;
            }

            std::string existingTime = checkQuery.getColumn("lastModifiedTime").getString();

            if (existingTime != FileUtils::getFileLastModifiedTime(path)) {

                return SongHasImportRes::NeedUpdate;
            } else {
                return SongHasImportRes::HasImport;
            }
        } else {
            return SongHasImportRes::No;
        }

    } catch (SQLite::Exception& e) {
        spdlog::get(LogDllID)->error("SongsManage::checkSongHasImport throw db error:{}", e.what());
        return SongHasImportRes::No;
    }
}

bool SongImport::importSingleSong(
    SongHasImportRes updateOrNoInTable,
    const juce::File& path,
    bool isSingleFile
) {

    // songImportPool.addJob([=, this] {
    auto result = insertLightSongInfo(path, isSingleFile, updateOrNoInTable);
    if (result.has_value()) {
        auto successRst{result.value()};
        auto& songInfo{successRst.info};

        // 检查并插入专辑
        //
        // 导入阶段**不再联网**：专辑信息只用本地标签。
        // 没有专辑名就不建专辑，歌曲继续用占位 albumId，等用户手动
        // 「联网搜索专辑封面」或者补标签再说（见 AlbumsManage::searchAndSaveCover）。
        std::optional<std::vector<std::string>> albumArtists{successRst.albumArtists};
        if (!albumArtists.has_value()) albumArtists = songInfo.artists;

        if (songInfo.album.has_value() && albumArtists.has_value()) {
            std::optional<int64_t> albumId;
            albumId = mDbContext.albumsManage.checkAlbumExist(
                songInfo.album.value(),
                albumArtists.value(),
                songInfo.hash
            );

            // 这次是不是新插了一张专辑：是的话落库完要通知前端加进专辑列表
            std::optional<AlbumInfo> newAlbum;
            if (!albumId.has_value()) {
                albumId = mDbContext.albumsManage.insertAlbum(
                    songInfo.album.value(),
                    albumArtists.value(),
                    songInfo.hash
                );

                if (albumId.has_value()) {
                    newAlbum = AlbumInfo{};
                    newAlbum->albumId = albumId.value();
                    newAlbum->title = songInfo.album.value();
                    newAlbum->albumArtists = albumArtists;
                    newAlbum->hash = songInfo.hash;
                }
            }

            mDbContext.dbManage.write([&](SQLite::Database& db) {
                SQLite::Statement updateAlbumId{
                    db,
                    "UPDATE songs SET albumId=:albumId WHERE songId=:songId"
                };
                DbUtils::bindOptValue(updateAlbumId, ":albumId", albumId);
                updateAlbumId.bind(":songId", songInfo.songId);
                updateAlbumId.exec();
            });

            if (newAlbum.has_value() && onAddNewAlbum) {
                onAddNewAlbum(ConvertUtils::object2Uint8t(newAlbum->toBasicJson()));
            }
        }

        // 歌词：找同目录同名 .lrc（顺手带翻译），纯文件操作，很快
        linkLyrics(songInfo.songId, path);

        return true;
    }
    return false;
    // });
}

void SongImport::linkLyrics(int64_t songId, const juce::File& audioPath) {
    if (songId <= 0) return;

    const auto directory = audioPath.getParentDirectory();
    const auto baseName = audioPath.getFileNameWithoutExtension();

    const auto originalFile = directory.getChildFile(baseName + ".lrc");
    if (!originalFile.existsAsFile()) return;

    const auto originalText = originalFile.loadFileAsString();
    if (originalText.isEmpty()) return;

    const auto originalMtime = originalFile.getLastModificationTime().toMilliseconds();

    mDbContext.lyricsManage.upsert(
        songId,
        LyricsManage::kKindOriginal,
        "lrc",
        originalText.toStdString(),
        originalFile.getFullPathName().toStdString(),
        originalMtime
    );

    // 翻译：常见后缀都试一遍，命中就存
    for (const auto* suffix : {".zh", ".cn", ".trans", ".translation"}) {
        const auto translationFile = directory.getChildFile(baseName + suffix + ".lrc");
        if (!translationFile.existsAsFile()) continue;

        const auto translationText = translationFile.loadFileAsString();
        if (translationText.isEmpty()) continue;

        mDbContext.lyricsManage.upsert(
            songId,
            LyricsManage::kKindTranslation,
            "lrc",
            translationText.toStdString(),
            translationFile.getFullPathName().toStdString(),
            translationFile.getLastModificationTime().toMilliseconds()
        );
        break;
    }

    if (auto logger = spdlog::get(LogDllID))
        logger->debug(
            "已关联歌词:songId={}, file={}",
            songId,
            originalFile.getFileName().toStdString()
        );
}

// ⚠️ 僵尸函数：目前**没有任何地方调用**它。
//
// 时域图（128 根柱子的波形）已经从产品里删掉了，但这里保留了完整的
// 解码 + 重采样流程（统一成 11025Hz / 单声道 / S16P），方便以后接
// AI 音频分析时直接把分析逻辑插进解码循环。
//
// 以后要用的时候：
//   1. 在 pushData 的位置（拿到 outputArray + 样本数）插入分析逻辑；
//   2. 外部接口单独抽一个任务结构体，不要复用这个只有 path 的；
//   3. 记得把开销放到非实时线程里。
void SongImport::waveFormAnaly(WaveFormAnalyTask task) {

    int result{0}; // 解码层结果，一般成功返回零
    av_log_set_level(AV_LOG_ERROR);
    AVFormatContext* inputContext{nullptr};
    avformat_open_input(&inputContext, task.path.c_str(), nullptr, nullptr);
    avformat_find_stream_info(inputContext, nullptr);

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];

    auto* decoderPar = pAudioStream->codecpar;

    auto codec = avcodec_find_decoder(decoderPar->codec_id);   // 根据ID寻找解码器
    auto* decoderContext = avcodec_alloc_context3(codec);      // 分配解码器上下文空间
    avcodec_parameters_to_context(decoderContext, decoderPar); // 连接参数和上下文
    avcodec_open2(decoderContext, codec, nullptr);             // 解码器初始化

    SwrContext* swrContext{nullptr};
    AVChannelLayout mono = AV_CHANNEL_LAYOUT_MONO;
    const int targetSampleRate{11025};
    swr_alloc_set_opts2( // 下面初始化重采样器
            &swrContext, 
            &mono,//单通道 
            AV_SAMPLE_FMT_S16P,
            targetSampleRate, //Chromaprint 输出音频采样率必须为11025
            &decoderPar->ch_layout,
            static_cast<AVSampleFormat>(decoderPar->format), 
            decoderPar->sample_rate, 
            0, 
            nullptr
        );
    swr_init(swrContext);

    auto* packet = av_packet_alloc();
    auto* frame = av_frame_alloc();

    while (av_read_frame(inputContext, packet) == 0) {

        if (packet->stream_index != currentIndex) {
            av_packet_unref(packet);
            continue;
        }
        result = avcodec_send_packet(decoderContext, packet);
        av_packet_unref(packet);
        if (result != 0) continue;
        while (avcodec_receive_frame(decoderContext, frame) == 0) {
            auto numInputSamples{
                swr_get_delay(swrContext, decoderPar->sample_rate) + frame->nb_samples
            };

            auto numOutputSamples{av_rescale_rnd(
                numInputSamples,
                targetSampleRate,
                decoderPar->sample_rate,
                AV_ROUND_UP
            )};

            uint8_t* outputArray{nullptr};
            result = av_samples_alloc(
                &outputArray, // 输出通道指针数组的起始地址
                nullptr,      // 物理字节大小
                1,
                numOutputSamples,
                AV_SAMPLE_FMT_S16P, // 指纹提取要求单通道且原始PCM格式
                0
            );

            if (result < 0) {
                av_frame_unref(frame);
                av_freep(&outputArray);
                continue;
            }

            int num4SwrSamples = swr_convert(
                swrContext,
                &outputArray,     // 这里面装的就是重采样后的PCM数据
                numOutputSamples, // 输出缓冲区的最大容量
                static_cast<uint8_t**>(frame->extended_data),
                frame->nb_samples
            ); // 执行重采样，返回实际重采样完的样本点个数

            if (num4SwrSamples < 0) {
                av_frame_unref(frame);
                av_freep(&outputArray);
                continue;
            }

            // TODO(AI): 以后的分析逻辑插在这里，outputArray 里是 11025Hz/单声道/S16P

            av_frame_unref(frame);

            av_freep(&outputArray);
        }
    }

    // 第一次解码器flush
    avcodec_send_packet(decoderContext, nullptr);
    while (avcodec_receive_frame(decoderContext, frame) == 0) {
        auto numInputSamples{
            swr_get_delay(swrContext, decoderPar->sample_rate) + frame->nb_samples
        };

        auto numOutputSamples{
            av_rescale_rnd(numInputSamples, targetSampleRate, decoderPar->sample_rate, AV_ROUND_UP)
        };

        uint8_t* outputArray{nullptr};
        result = av_samples_alloc(
            &outputArray, // 输出通道指针数组的起始地址
            nullptr,      // 物理字节大小
            1,
            numOutputSamples,
            AV_SAMPLE_FMT_S16P,
            0
        ); // 第二个参数和第四个参数的区别：nb_samples 是逻辑样本数（比如 1024 个浮点样本）。
        // linesize 是物理字节大小（比如对于 1024 个浮点样本，linesize 通常是 1024 * 4 = 4096
        // 字节，但如果内存对齐强制要求 64 字节对齐，它可能是 4096 或 4096+）。

        if (result < 0) {
            av_frame_unref(frame);
            av_freep(&outputArray);
            continue;
        }

        result = swr_convert(
            swrContext,
            &outputArray,     // 这里面装的就是重采样后的PCM数据
            numOutputSamples, // 输出缓冲区的最大容量(理论最大值)
            static_cast<uint8_t**>(frame->extended_data),
            frame->nb_samples
        ); // 执行重采样，返回实际重采样完的样本点个数

        if (result <= 0) {
            av_freep(&outputArray);
            continue;
        }

        // TODO(AI): 解码器 flush 出来的尾部样本，同上
        av_frame_unref(frame);

        av_freep(&outputArray);
    }

    // 第二次重采样器flush
    int maxFlushSamples = swr_get_out_samples(swrContext, 0);
    // 参数 0 表示“不喂入任何新输入，只计算内部残余需要多少输出空间”

    if (maxFlushSamples > 0) {
        uint8_t* outputArray = nullptr;
        av_samples_alloc(&outputArray, nullptr, 1, maxFlushSamples, AV_SAMPLE_FMT_S16P, 0);

        while (true) {
            int ret = swr_convert(swrContext, &outputArray, maxFlushSamples, nullptr, 0);
            if (ret <= 0) break;
            // TODO(AI): 重采样器 flush 的残余样本，同上
        }
        // 释放 Flush 临时缓冲区
        if (outputArray != nullptr) {
            av_freep(&outputArray);
        }
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swrContext);
    avcodec_free_context(&decoderContext);
}

std::optional<SongImport::InsertLightSongInfoResult> SongImport::insertLightSongInfo(
    const juce::File& path,
    bool isSingleFile,
    SongHasImportRes songHasImportRes
) {
    SongInfo info{};
    InsertLightSongInfoResult result;
    av_log_set_level(AV_LOG_ERROR);
    std::string filePath = path.getFullPathName().toStdString();
    std::string lastModifiedTime = FileUtils::getFileLastModifiedTime(path);

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

            return std::nullopt;
        }
    }
    {
        int avResult = avformat_find_stream_info(inputContext, nullptr);
        if (avResult < 0) {
            if (logger)
                logger->error("无法找到该文件的流信息:{}", OtherUtils::ffmpegErrorOutput(avResult));
            avformat_close_input(&inputContext);

            return std::nullopt;
        }
    }
    // length(seconds)
    if (inputContext->duration != AV_NOPTS_VALUE) {
        info.duration = static_cast<double>(inputContext->duration) / AV_TIME_BASE;
    }

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    if (currentIndex < 0) {
        avformat_close_input(&inputContext);
        return std::nullopt;
    }

    auto* pAudioStream = inputContext->streams[currentIndex];
    auto* decoderPar = pAudioStream->codecpar;

    info.codecName = avcodec_get_name(decoderPar->codec_id);

    info.bitRate = decoderPar->bit_rate / 1000;

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
            auto titles = mDbContext.songsManage.getTags(map, "TITLE");
            if (!titles.empty()) {
                info.title = titles[0];
                result.titleIsFileName = false;
            } else {
                info.title = defaultTitle;
                result.titleIsFileName = true;
            }

            // Artist
            auto artists = mDbContext.songsManage.getTags(map, "ARTIST");
            if (!artists.empty()) {
                info.artists = artists;
            }

            // Album
            auto albums = mDbContext.songsManage.getTags(map, "ALBUM");
            if (!albums.empty()) {
                info.album = albums[0];
            }

            {
                auto albumArtists{mDbContext.songsManage.getTags(map, "ALBUMARTIST")};
                if (!albumArtists.empty()) {
                    result.albumArtists = albumArtists;
                }
            }

            // 数值型信息
            info.trackNumber = mDbContext.songsManage.getTagInt(map, "TRACKNUMBER");
            info.discNumber = mDbContext.songsManage.getTagInt(map, "DISCNUMBER");

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

                juce::File hashImageDir{mImageDir.getChildFile(info.hash.value())};
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

    // ── 单独导入的歌曲：把音频文件拷进缓存，之后它的「家」就是缓存副本 ──
    // 文件夹导入的歌曲保持原样（filePath = 原路径，文件留在用户磁盘上）
    juce::File storedFile = path;
    bool copiedToCache = false;
    if (isSingleFile) {
        storedFile = getCachedFileFor(path);
        const bool needCopy =
            songHasImportRes != SongHasImportRes::HasImport || !storedFile.existsAsFile();

        if (needCopy) {
            if (!mSongsDir.isDirectory() && !mSongsDir.createDirectory()) {
                if (logger)
                    logger->error(
                        "歌曲缓存目录创建失败，放弃导入:{} / {}",
                        mSongsDir.getFullPathName().toStdString(),
                        filePath
                    );
                return std::nullopt;
            }

            // 先写 .tmp 再改名：中途失败/断电都不会留下半个文件
            const juce::File tmpFile = storedFile.getSiblingFile(storedFile.getFileName() + ".tmp");
            if (tmpFile.existsAsFile()) tmpFile.deleteFile();

            bool copyOk = path.copyFileTo(tmpFile);
            if (copyOk) {
                if (storedFile.existsAsFile() && !storedFile.deleteFile()) copyOk = false;
                if (copyOk) copyOk = tmpFile.moveFileTo(storedFile);
            }
            if (!copyOk) {
                tmpFile.deleteFile();
                if (logger) logger->error("拷贝歌曲到缓存失败，放弃导入:{}", filePath);
                return std::nullopt;
            }
            copiedToCache = true;
        }
    }

    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&](SQLite::Statement& stmt) {
        // ── 必选字段 ──
        stmt.bind(":filePath", storedFile.getFullPathName().toStdString());
        stmt.bind(":isSingleFile", isSingleFile ? 1 : 0);
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

        mDbContext.dbManage.write([&](SQLite::Database& db) {
            if (songHasImportRes == SongHasImportRes::NeedUpdate) {
                // ── 文件已变更：更新 songs 记录 ──
                SQLite::Statement updateSong(
                    db,
                    "UPDATE songs SET  "
                    "lastModifiedTime = :lastModifiedTime, isSingleFile = :isSingleFile, "
                    "duration = :duration, title = :title, artists = :artists, album = :album, "
                    "trackNumber = :trackNumber,  discNumber = :discNumber,"
                    "bitRate = :bitRate, bitDepth = :bitDepth, hash = :hash, "
                    "sampleRate = :sampleRate, channelLayoutMask = :channelLayoutMask, "
                    "codecName = :codecName "
                    "WHERE filePath = :filePath RETURNING songId"
                );

                bindSongFields(updateSong);

                updateSong.executeStep();
                info.songId = updateSong.getColumn("songId").getInt64();

            } else if (songHasImportRes == SongHasImportRes::No) {
                // ── 新文件：插入 songs 记录 ──
                SQLite::Statement insert(
                    db,
                    "INSERT INTO songs (filePath, lastModifiedTime, isSingleFile, "
                    "duration, title, artists, album, "
                    "trackNumber, discNumber,bitRate, bitDepth, "
                    "sampleRate, channelLayoutMask, codecName, hash) "

                    " VALUES (:filePath, :lastModifiedTime, :isSingleFile, :duration, :title, "
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

            if (!isSingleFile) {
                SQLite::Statement checkDirStmt{db, "SELECT id,dir FROM songFolders WHERE dir=:dir"};
                checkDirStmt.bind(
                    ":dir",
                    path.getParentDirectory().getFullPathName().toStdString()
                );
                if (checkDirStmt.executeStep()) {
                    // std::string dirStr = checkDirStmt.getColumn("dir").getString();
                    auto songFolderId = checkDirStmt.getColumn("id").getInt64();
                    SQLite::Statement updateDirIdStmt{
                        db,
                        "UPDATE songs SET songFolderId=:songFolderId WHERE songId=:songId"
                    };
                    updateDirIdStmt.bind(":songFolderId", songFolderId);
                    updateDirIdStmt.bind(":songId", info.songId);
                    updateDirIdStmt.exec();
                }
            }
        });
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("insert song error:{}", e.what());

        // 已经拷进缓存的副本没有对应记录了，删掉，别留垃圾
        if (copiedToCache && storedFile.existsAsFile()) storedFile.deleteFile();

        return std::nullopt;
    }

    result.info = std::move(info);
    result.finalPath = storedFile;
    result.copiedToCache = copiedToCache;
    if (onAddOrUpdateSong)
        onAddOrUpdateSong(ConvertUtils::object2Uint8t(result.info.toBasicJson()));

    return result;
}

void SongImport::importSongs(Yvar songPaths) {
    for (int i = 0; i < songPaths.size(); i++) {
        auto file = juce::File{songPaths.read(i).toString()};

        // 单独导入：查重用的是「算出来的缓存路径」，不是原路径
        auto checkRes = checkSongHasImport(file, true);
        if (checkRes == SongHasImportRes::HasImport) continue;

        importSingleSong(checkRes, file, true);
    }
}
