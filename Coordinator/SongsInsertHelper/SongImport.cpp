#include "./SongImport.hpp"
#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/convertUtils.hpp"
#include "Utils/dbUtils.hpp"
#include "Utils/fileUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "Utils/stringUtils.hpp"
#include "juce_core/juce_core.h"
#include <chromaprint.h>
#include <cpr/api.h>
#include <cpr/response.h>
#include <cpr/timeout.h>
#include <cstddef>
#include <fileref.h>
#include <sha1.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

void SongImport::checkAndInsertSong(const juce::File& path, bool isSingleFile) {

    songImportPool.addJob([=, this] {
        auto isSongIntable = mDbContext.songsManage.checkSongHasImport(path);
        if (isSongIntable == SongsManage::SongHasImportRes::HasImport) {
            return;
        }

        auto result = insertLightSongInfo(path, isSingleFile, isSongIntable);
        if (result.has_value()) {
            auto successRst{result.value()};
            auto& songInfo{successRst.info};

            // 检查并插入专辑

            bool needSearchAlbumArtists{false};
            if (successRst.albumArtists.has_value()) {
                needSearchAlbumArtists = true;
            }
            bool needSearchSongTitle{false};
            if (successRst.titleIsFileName) {
                needSearchSongTitle = true;
            }
            bool needSearchArtists(false);
            if (!songInfo.artists.has_value()) {
                needSearchArtists = true;
            }
            bool needSearchAlbum{false};
            if (!songInfo.album) {
                needSearchAlbum = true;
            }
            bool needSearchCoverHash{false};
            if (!songInfo.hash) {
                needSearchCoverHash = true;
            }
            std::optional<std::vector<std::string>> albumArtists;
            if (successRst.albumArtists.has_value()) {
                albumArtists = successRst.albumArtists.value();
            } else {
                albumArtists = songInfo.artists;
            }

            bool needInsertOrCheckAlbum{false}; // 是否需要立刻插入专辑，而不是搜索后再插入
            if (!needSearchAlbum && !needSearchArtists) {
                needInsertOrCheckAlbum = true;
            }

            if (needInsertOrCheckAlbum) {
                std::optional<int64_t> albumId;
                albumId = mDbContext.albumsManage.checkAlbumExist(
                    songInfo.album.value(),
                    albumArtists.value(),
                    songInfo.hash
                );
                if (!albumId.has_value()) {
                    albumId = mDbContext.albumsManage.insertAlbum(
                        songInfo.album.value(),
                        albumArtists.value(),
                        songInfo.hash
                    );
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
            }

            // return songInfo;

            // 波形获取
            waveFormAnalyPool.addJob([=, this] {
                auto& songInfo{successRst.info};

                bool needSearchSongInfo{false}; // 是否需要联网搜索歌曲信息
                if (needSearchSongTitle || needSearchAlbum || needSearchArtists ||
                    needSearchCoverHash) {
                    needSearchSongInfo = true;
                }

                auto waveFormAnalyResult =
                    waveFormAnaly({path.getFullPathName().toStdString(), needSearchSongInfo});

                OnlineSearchRes onlineGetMetadataResult;
                if (needSearchSongInfo && waveFormAnalyResult.print.has_value()) {
                    auto print{waveFormAnalyResult.print.value()};

                    // 专辑艺术家如果没有不需要联网搜索，因为如果要搜索专辑艺术家的话每首歌都需要这么做
                    onlineGetMetadataResult =  onlineSearch(
                        {
                            songInfo.songId,
                            songInfo.duration,
                            std::move(print),
                            successRst.titleIsFileName ? std::nullopt
                                                       : std::optional<std::string>{songInfo.title},
                                                       songInfo.artists,
                                                       songInfo.album,
                                                       needSearchCoverHash

                        }
                    );
                }
                try {

                    mDbContext.dbManage.write([&](SQLite::Database& db) {
                        SQLite::Transaction t{db};
                        SQLite::Statement sql1{
                            db,
                            "UPDATE songs SET "
                            "title=COALESCE(:title,title),artists=COALESCE(:artists,artists),album="
                            "COALESCE(:album,album),hash=COALESCE(:hash,hash),timeDomainSpec=:"
                            "timeDomainSpec WHERE songId=:songId"
                        };

                        if (needSearchSongTitle) {
                            DbUtils::bindOptValue(
                                sql1,
                                ":title",
                                onlineGetMetadataResult.finalTitle
                            );
                        }
                        if (needSearchArtists) {
                            DbUtils::bindOptValue(
                                sql1,
                                ":artists",
                                onlineGetMetadataResult.finalArtists
                            );
                        }

                        if (needSearchAlbum) {
                            DbUtils::bindOptValue(
                                sql1,
                                ":album",
                                onlineGetMetadataResult.finalAlbum
                            );
                        }

                        if (needSearchCoverHash) {
                            DbUtils::bindOptValue(sql1, ":hash", onlineGetMetadataResult.finalHash);
                        }

                        sql1.bind(
                            ":timeDomainSpec",
                            waveFormAnalyResult.blobBuffer.data(),
                            static_cast<int>(waveFormAnalyResult.blobBuffer.size() * sizeof(double))
                        );
                        sql1.bind(":songId", songInfo.songId);
                        sql1.exec();

                        if (needSearchAlbum || needSearchAlbumArtists || needSearchCoverHash) {
                            auto albumId = mDbContext.albumsManage.getAlbumId(songInfo.songId);
                            if (albumId.has_value()) {

                                SQLite::Statement sql2{
                                    db,
                                    "UPDATE albums SET "
                                    "title=COALESCE(:title,title),albumArtists=COALESCE(:"
                                    "albumArtists,"
                                    "albumArtists),hash=COALESCE(:hash,hash) WHERE albumId=:albumId"
                                };

                                DbUtils::bindOptValue(
                                    sql2,
                                    ":title",
                                    onlineGetMetadataResult.finalAlbum
                                );
                                DbUtils::bindOptValue(
                                    sql2,
                                    ":albumArtists",
                                    onlineGetMetadataResult.finalAlbumArtists.has_value()
                                        ? onlineGetMetadataResult.finalAlbumArtists
                                        : onlineGetMetadataResult.finalArtists
                                );
                                DbUtils::bindOptValue(
                                    sql2,
                                    ":hash",
                                    onlineGetMetadataResult.finalHash
                                );
                                DbUtils::bindOptValue(sql2, ":albumId", albumId);
                                sql2.exec();
                            }
                        }
                        t.commit();
                    });
                    if (onUpdateSongInfo)
                        onUpdateSongInfo(
                            ConvertUtils::object2Uint8t(mDbContext.songsManage
                                                            .getBasicSongInfo(songInfo.songId)
                                                            .toBasicJson())
                        );
                } catch (SQLite::Exception& e) {
                    spdlog::get(LogDllID)->error(
                        "dllManager::insertSong发生数据库错误:{}",
                        e.what()
                    );
                }
            });

            return;
        }
        return;
    });
}

SongImport::WaveFormAnalyRes SongImport::waveFormAnaly(WaveFormAnalyTask task) {

    int result{0}; // 解码层结果，一般成功返回零
    av_log_set_level(AV_LOG_ERROR);
    AVFormatContext* inputContext{nullptr};
    avformat_open_input(&inputContext, task.path.c_str(), nullptr, nullptr);
    avformat_find_stream_info(inputContext, nullptr);

    auto currentIndex{av_find_best_stream(inputContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0)};

    auto* pAudioStream = inputContext->streams[currentIndex];

    // 时长（秒）
    double duration{0.0f};
    if (pAudioStream->duration != AV_NOPTS_VALUE) {
        duration = pAudioStream->duration * av_q2d(pAudioStream->time_base);
    } else if (inputContext->duration != AV_NOPTS_VALUE) {
        duration = inputContext->duration / (double)AV_TIME_BASE;
    }
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

    std::array<double, 128> blobBuffer{}; // 存入数据库的波形图数组,前端显示128根柱子

    int64_t totalSamples{static_cast<int64_t>(duration * (double)targetSampleRate)};

    auto samplePerLeftBin{static_cast<int64_t>(totalSamples / 128)}; // 每根柱子容纳多少个采样点
    auto samplePerRightBin{
        samplePerLeftBin + 1
    }; // 靠右的格子为靠左的加一，因为总数可能不会被128整除
    auto num4LeftBin{samplePerRightBin * 128 - totalSamples}; // 左边柱子的数目

    int binCount{0};         // 帧循环中使用了多少根柱子
    int sampleCount{0};      // 每个柱子中已经存储了多少个样本点
    double squarePlus{0.0f}; // 平方和

    auto pushDataIntoAudioGraph = [&](uint8_t* outputArray, int swrResult) {
        const int16_t* src = reinterpret_cast<const int16_t*>(outputArray);
        for (int i = 0; i < swrResult; i++) {
            if (binCount >= 128) return;
            double value{(double)src[i] / 32767.0};
            auto square = value * value;
            squarePlus += square;
            sampleCount++;
            if (binCount < num4LeftBin) {
                if (sampleCount >= samplePerLeftBin) {

                    sampleCount = 0;

                    auto rms{std::sqrt(squarePlus / samplePerLeftBin)};
                    blobBuffer[binCount] = rms;
                    binCount++;
                    squarePlus = 0.0f;
                }
            } else {
                if (sampleCount >= samplePerRightBin) {
                    sampleCount = 0;

                    auto rms{std::sqrt(squarePlus / samplePerRightBin)};
                    blobBuffer[binCount] = rms;
                    binCount++;
                    squarePlus = 0.0f;
                }
            }
        }
    };

    auto* packet = av_packet_alloc();
    auto* frame = av_frame_alloc();

    // bool needOnlineSearch{
    //     task.onlineTask.album.isEmpty() || task.onlineTask.needCover ||
    //     task.onlineTask.artists.isEmpty() || task.onlineTask.title.isEmpty()
    // }; // 是否需要联网搜索,四个里面有一个不存在就需要

    // bool needOnlineSearch{true}; // 这个测试用

    ChromaprintContext* printContext{nullptr};
    if (task.needGetPrint) {
        // 音频指纹提取初始化
        printContext = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
        chromaprint_start(printContext, targetSampleRate, 1); // 指纹提取强制这个格式
    }
    double currentSeconds{0.0}; // 指纹提取只提取前90秒

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

            pushDataIntoAudioGraph(outputArray, num4SwrSamples);

            currentSeconds += (double)frame->nb_samples / (double)decoderPar->sample_rate;

            av_frame_unref(frame);

            // 喂入指纹上下文
            if (task.needGetPrint && currentSeconds < 90.0)
                chromaprint_feed(printContext, (int16_t*)outputArray, num4SwrSamples);

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

        pushDataIntoAudioGraph(outputArray, result);
        currentSeconds += (double)frame->nb_samples / (double)decoderPar->sample_rate;
        av_frame_unref(frame);
        if (task.needGetPrint && currentSeconds < 90.0)
            chromaprint_feed(printContext, (int16_t*)outputArray, result);

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
            pushDataIntoAudioGraph(outputArray, ret);

            currentSeconds += (double)ret / (double)decoderPar->sample_rate;
            if (task.needGetPrint && currentSeconds < 90.0)
                chromaprint_feed(printContext, (int16_t*)outputArray, ret);
        }
        // 释放 Flush 临时缓冲区
        if (outputArray != nullptr) {
            av_freep(&outputArray);
        }
    }

    if (binCount < 128 && sampleCount > 0) {
        // 此时 squarePlus 里面装的就是最后剩下的那些样本的平方和
        blobBuffer[binCount] = std::sqrt(squarePlus / sampleCount);
        binCount++;
    }

    av_frame_free(&frame);
    av_packet_free(&packet);
    swr_free(&swrContext);
    avcodec_free_context(&decoderContext);

    std::string printStr;
    if (task.needGetPrint) {
        chromaprint_finish(printContext);

        char* printPtr{nullptr};
        if (chromaprint_get_fingerprint(printContext, &printPtr) == 1) {
            printStr = std::string{printPtr};
            chromaprint_dealloc(printPtr);
            // task.onlineTask.duration = duration;
            // task.onlineTask.print = std::move(printStr);
            // mOnlineGetMatedata.setTask(std::move(task.onlineTask));
        }
        chromaprint_free(printContext);
    }

    // try {
    //     dbManage.write([&](SQLite::Database& db) {
    //         SQLite::Statement sql(
    //             db,
    //             "UPDATE songs SET timeDomainSpec = :timeDomainSpec WHERE filePath = :filePath"
    //         );
    //         sql.bind(
    //             ":timeDomainSpec",
    //             blobBuffer.data(),
    //             static_cast<int>(blobBuffer.size() * sizeof(double))
    //         ); // 时域图
    //         sql.bind(":filePath", task.path);

    //         // std::string spdlogStr{""};
    //         // spdlog::get(LogDllID)->debug(
    //         //     "文件路径:{},波形图数组元素个数为:{},数组占用字节大小为{}",
    //         //     task.path,
    //         //     blobBuffer.size(),
    //         //     static_cast<int>(blobBuffer.size() * sizeof(double))
    //         // );
    //         // for (size_t i = 0; i < blobBuffer.size(); i++) {

    //         //     spdlogStr += std::to_string(blobBuffer[i]) + " ";
    //         // }
    //         // spdlog::get(LogDllID)->debug("波形图内容:{}", spdlogStr);

    //         sql.exec();
    //     });
    // } catch (SQLite::Exception& e) {
    //     spdlog::get(LogDllID)->error("WaveFormAnaly线程发生数据库错误:{}", e.what());
    // }
    // if (task.needGetPrint) {
    //     return std::move(printStr);
    // } else {
    //     return std::nullopt;
    // }
    WaveFormAnalyRes r;
    if (task.needGetPrint) {
        r.print = std::move(printStr);
    }
    r.blobBuffer = std::move(blobBuffer);
    return r;
}

std::optional<std::string> SongImport::searchCoverURL(std::string term, std::string entity) {
    spdlog::get(LogDllID)->debug("iTunes 搜索 term: {}, entity: {}", term, entity);
    cpr::Response coverSearch = cpr::Get(
        cpr::Url{"https://itunes.apple.com/search"},
        cpr::Parameters{{"term", term}, {"media", "music"}, {"entity", entity}, {"limit", "1"}},
        cpr::Timeout{10000}
    );

    if (coverSearch.status_code == 200) {
        Yvar coverSearchRes = Yvar::parse(coverSearch.text);
        if (coverSearchRes.read("resultCount").toInt() > 0) {
            auto firstResult = coverSearchRes.read("results").read(0);
            return firstResult.read("artworkUrl100").toOptStr();
        }
    }
    return std::nullopt;
}

SongImport::OnlineSearchRes SongImport::onlineSearch(SongImport::OnlineSearchTask task) {
    spdlog::get(LogDllID)->debug("开始根据指纹搜索元数据:歌曲ID:{}", task.songId);

    OnlineSearchRes onLineResult;

    spdlog::get(LogDllID)->debug(
        "已有数据内容为:songId:{},title:{},artists:{},album:{}",
        task.songId,
        task.title.has_value() ? task.title.value() : "",
        task.artists.has_value() ? StringUtils::vectorToString(task.artists.value(), " / ") : "",
        task.album.has_value() ? task.album.value() : ""
    );

    // 发起 POST 请求
    cpr::Response res = cpr::Post(
        cpr::Url{"https://api.acoustid.org/v2/lookup"},
        // cpr::Payload 会自动将参数放在 HTTP Body 中以 POST 表单发出
        cpr::Payload{
            {"client", AcoustIdKey},
            {"meta", "recordings + releases"},
            {"duration", std::to_string((int)task.duration)},
            {"fingerprint", task.print},
            {"format", "json"}
        },
        cpr::Timeout{5000} // 5秒超时
    );

    if (res.status_code == 200) {
        // spdlog::get(LogDllID)->debug("得到网络请求返回的表单数据:{}", res.text);

        struct RecordingResult { // 搜索到的recording结果
            double score{100.0}; // 最终匹配度得分
            int resultIndex{0};
            int recordingIndex{0};
        };
        std::vector<RecordingResult> recordingResults;

        Yvar v = Yvar::parse(res.text);
        const Yvar results = v.read("results");

        for (int resultIndex = 0; resultIndex < results.size(); resultIndex++) {
            const Yvar result = results.read(resultIndex);
            double printScore{result.read("score").toDouble()};
            // 原始分数
            if (printScore <= 0.7) {
                continue;
            } // 指纹匹配度小于0.7直接退出

            const Yvar recordings = result.read("recordings");
            for (int recordingIndex = 0; recordingIndex < recordings.size(); recordingIndex++) {

                RecordingResult recordingResult;
                recordingResult.score *= printScore;
                const Yvar recording = recordings.read(recordingIndex);
                auto recordingDuration{recording.read("duration").toDouble()};
                auto durationScore{1 - std::abs(recordingDuration - task.duration) / task.duration};
                if (durationScore <= 0.85) {
                    continue;
                } else {
                    recordingResult.score *= durationScore;
                } // 根据歌曲时长匹配分数

                if (task.title.has_value()) {
                    // 有原本的标题则进行相似度匹配
                    auto titleScore = StringUtils::stringSimilarity(
                        recording.read("title").toString(),
                        task.title.value()
                    );
                    if (titleScore > 0.7) {
                        recordingResult.score *= (1.0 + titleScore);
                    } else {
                        recordingResult.score *= 0.5;
                    }
                    // recordingResult.score *= titleScore;
                }
                if (task.artists.has_value()) {
                    const Yvar searchArtists = recording.read("artists"); // 搜索到的艺术家列表

                    std::vector<std::string> remoteArtistNames;
                    for (int i = 0; i < searchArtists.size(); i++) {
                        auto name = searchArtists.read(i).read("name").toString();
                        if (!name.empty()) remoteArtistNames.push_back(std::move(name));
                    }

                    if (!remoteArtistNames.empty()) {
                        double calcResult{StringUtils::calcArtistsSimlarity(
                            task.artists.value(),
                            remoteArtistNames
                        )};

                        // 乘入总分
                        recordingResult.score *= calcResult;

                    } else {
                        // 线上完全没有返回艺术家信息时，仅扣除 20% 分数
                        recordingResult.score *= 0.8;
                    }
                }
                recordingResult.resultIndex = resultIndex;
                recordingResult.recordingIndex = recordingIndex;

                recordingResults.push_back(recordingResult);
            }
        }
        // 得到最高分的recording结果
        auto bestRecordingIt = std::max_element(
            recordingResults.begin(),
            recordingResults.end(),
            [](const RecordingResult& a, const RecordingResult& b) {
                return a.score < b.score; // 比较：a < b 则 a 更小
            }
        );

        if (bestRecordingIt == recordingResults.end()) {
            spdlog::get(LogDllID)->debug("找不到合适的recording,提前返回");
            return {};
        }
        const Yvar finalRecording = results.read(bestRecordingIt->resultIndex)
                                        .read("recordings")
                                        .read(bestRecordingIt->recordingIndex);
        const Yvar releases = finalRecording.read("releases");

        spdlog::get(LogDllID)->debug("获取到的releases内容:{}", releases.toString());

        int bestReleaseIndex{0};
        struct ReleaseResult {
            int releaseindex{0};
            double score{100.0};
        };
        std::vector<ReleaseResult> releaseResults;
        // if (task.album.isNotEmpty()) {

        for (int releaseIndex = 0; releaseIndex < releases.size(); releaseIndex++) {
            const Yvar release = releases.read(releaseIndex);
            const Yvar remoteAlbumTitle = release.read("title");
            auto country = release.read("country").toString();
            StringUtils::toUpperCaseInPlace(country);

            ReleaseResult releaseResult;
            releaseResult.releaseindex = releaseIndex;

            if (task.album.has_value()) {
                auto albumScore{
                    StringUtils::stringSimilarity(task.album.value(), remoteAlbumTitle.toString())
                };
                if (albumScore > 0.7) {
                    releaseResult.score *= (1.0 + albumScore);
                } else {
                    releaseResult.score *= 0.5; // 这一步是为了防止不同语言但是又是同一张专辑的情况
                }
                // releaseResult.score *= albumScore;
            }

            if (country == mSystemCountry) {
                releaseResult.score *= 1.2;
            } else if (country == "XW") {
                // 全球发行版本
                releaseResult.score *= 1.1;
            } else if (country == "XE" || country == "EU") {
                // 欧洲统一发行版本
                releaseResult.score *= 1.05;
            } else {
                releaseResult.score *= 0.95;
            }

            releaseResults.push_back(releaseResult);
        }

        auto bestReleaseIt = std::max_element(
            releaseResults.begin(),
            releaseResults.end(),
            [](const ReleaseResult& a, const ReleaseResult& b) {
                return a.score < b.score; // 比较：a < b 则 a 更小
            }
        );
        if (bestReleaseIt == releaseResults.end()) {
            spdlog::get(LogDllID)->debug("找不到合适的release，提前return");
            return {};
        }
        bestReleaseIndex = bestReleaseIt->releaseindex;

        auto bestTitle = finalRecording.read("title").toString();

        const Yvar bestArtists = finalRecording.read("artists");
        std::vector<std::string> bestArtistsArray;

        for (int i = 0; i < bestArtists.size(); i++) {
            auto artist = bestArtists.read(i).read("name").toString();
            if (!artist.empty()) bestArtistsArray.push_back(std::move(artist));
        }

        const Yvar finalRelease = finalRecording.read("releases").read(bestReleaseIndex);

        std::string bestAlbum = finalRelease.read("title").toString();

        spdlog::get(LogDllID)->debug(
            "id:{}:最终联网搜索获得的歌名:{},艺术家组合名:{},专辑名:{}",
            task.songId,
            bestTitle,
            StringUtils::vectorToString(bestArtistsArray, " / "),
            bestAlbum
        );
        std::string bestHash;
        std::vector<std::string> bestAlbumArtists;
        const Yvar finalAlbumArtistsVar = finalRelease.read("artists");
        for (int i = 0; i < finalAlbumArtistsVar.size(); i++) {
            auto name = finalAlbumArtistsVar.read(i).read("name").toString();
            if (!name.empty()) bestAlbumArtists.push_back(std::move(name));
        }

        if (task.needCover) {

            std::string primaryArtist;
            for (size_t i = 0; i < bestAlbumArtists.size(); i++) {
                if (!StringUtils::isVariousArtists(bestAlbumArtists[i])) {
                    // 如果是群星等名字不能发给apple做搜索，否则极大降低成功率
                    primaryArtist = bestAlbumArtists[i];
                    break; // 搜索专辑图只需主艺术家名
                }
            }

            if (primaryArtist.empty() && !bestArtistsArray.empty()) {
                primaryArtist = bestArtistsArray[0];
            } // 如果专辑艺术家不可用的话，发送歌曲艺术家

            auto searchTerms{primaryArtist + " " + bestAlbum};
            auto coverURL{searchCoverURL(searchTerms, "album")};
            if (coverURL == std::nullopt) {
                searchTerms = primaryArtist + " " + bestTitle;
                coverURL = searchCoverURL(searchTerms, "song");
                // 艺术家加上专辑名走不到就改用歌曲名称
                if (coverURL == std::nullopt) {
                    spdlog::get(LogDllID)->info("搜不到专辑图片:歌曲ID{}", task.songId);
                    return {};
                }
            }

            // 替换 URL 获取 600x600 或 1000x1000 高清大图
            size_t pos = coverURL.value().find("100x100bb.jpg");
            if (pos != std::string::npos) {
                coverURL.value().replace(pos, 13, "1000x1000bb.jpg"); // 替换为目标分辨率
            }

            // 5. 发起第二次请求：真正下载图片数据
            cpr::Response imgRes = cpr::Get(cpr::Url{coverURL.value()}, cpr::Timeout{10000});

            if (imgRes.status_code == 200) {

                do {
                    SHA1 sha1;
                    auto hash = sha1(imgRes.text.data(), imgRes.text.size());
                    spdlog::get(LogDllID)->debug("联网搜索到图片哈希值:{}", hash);
                    bestHash = std::string(hash);

                    juce::File hashImageDir{mImageDir.getChildFile(hash)};

                    if (hashImageDir.exists()) {
                        break;
                    } else {
                        hashImageDir.createDirectory();
                    } // 如果这个目录已经存在，直接退出，避免保存两个相同图片

                    juce::File originalFile{hashImageDir.getChildFile("original.jpg")};

                    juce::FileOutputStream outputStream(originalFile);
                    if (outputStream.openedOk()) {
                        outputStream.write(imgRes.text.data(), imgRes.text.size());
                        outputStream.flush();
                    }

                } while (0);

            } else {
                spdlog::get(LogDllID)->warn("封面图片下载失败，状态码: {}", imgRes.status_code);
            }
        }
        spdlog::get(LogDllID)->debug("联网搜索歌曲元数据完成，歌曲ID:{}", task.songId);

        if (!task.title.has_value() && !bestTitle.empty()) {
            onLineResult.finalTitle = bestTitle;
        }
        if (!task.artists.has_value() && !bestArtistsArray.empty()) {
            onLineResult.finalArtists = bestArtistsArray;
        }
        if (!task.album.has_value() && !bestAlbum.empty()) {
            onLineResult.finalAlbum = bestAlbum;
        }
        if (task.needCover && !bestHash.empty()) {
            onLineResult.finalHash = bestHash;
        }
        if (!bestAlbumArtists.empty()) {
            onLineResult.finalAlbumArtists = bestAlbumArtists;
        }
        return onLineResult;

    } else {
        spdlog::get(LogDllID)
            ->error("请求失败！错误码: {} | 服务器返回的错误信息: {}", res.status_code, res.text);

        return {};
    }
}

std::optional<SongImport::InsertLightSongInfoResult> SongImport::insertLightSongInfo(
    const juce::File& path,
    bool isSingleFile,
    SongsManage::SongHasImportRes songHasImportRes
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

    auto* pAudioStream = inputContext->streams[currentIndex];
    auto* decoderPar = pAudioStream->codecpar;

    if (currentIndex < 0) {
        avformat_close_input(&inputContext);

        return std::nullopt;
    }

    info.codecName = avcodec_get_name(decoderPar->codec_id);

    // 比特率（kbps）——先取编码器报告值，缺失时用文件大小估算
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

    // ── 绑定 songs 表字段的辅助 lambda（复用 INSERT 和 UPDATE 两处） ──
    auto bindSongFields = [&](SQLite::Statement& stmt) {
        // ── 必选字段 ──
        stmt.bind(":filePath", filePath);
        stmt.bind(":isSingleFile", isSingleFile);
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
            if (songHasImportRes == SongsManage::SongHasImportRes::NeedUpdate) {
                // ── 文件已变更：更新 songs 记录 ──
                SQLite::Statement updateSong(
                    db,
                    "UPDATE songs SET   isSingleFile=:isSingleFile"
                    "lastModifiedTime = :lastModifiedTime, "
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

            } else if (songHasImportRes == SongsManage::SongHasImportRes::No) {
                // ── 新文件：插入 songs 记录 ──
                SQLite::Statement insert(
                    db,
                    "INSERT INTO songs (filePath, lastModifiedTime, isSingleFile"
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
        });
    } catch (const SQLite::Exception& e) {
        if (logger) logger->error("insert song error:{}", e.what());

        return std::nullopt;
    }

    result.info = std::move(info);
    if (onAddNewSong) onAddNewSong(ConvertUtils::object2Uint8t(result.info.toBasicJson()));

    return result;
}

void SongImport::importSingleSongs(Yvar songPaths) {
    for (int i = 0; i < songPaths.size(); i++) {
        auto file = juce::File{songPaths.read(i).toString()};
        checkAndInsertSong(file, true);
    }
}
void SongImport::importNewFolder(const juce::File& folderPath) {
    if (mDbContext.songFolderManage.isDirHasImport(folderPath)) {
        if (mFolderImportIsRoot == 0) {
            // 如果是文件导入递归的第一层，直接发送错误信号给前端，通知这个文件夹已经导入，不要再导入了
            if (onDirHasRegister)
                onDirHasRegister(
                    std::format(
                        "文件夹{}已经导入，请勿重复操作",
                        folderPath.getFullPathName().toStdString()
                    )
                );
        }
        return;
    } else {
        if (mFolderImportIsRoot == 0) {
            // 递归层数为0说明这个文件夹需要注册为根目录
            mDbContext.songFolderManage.insertFolderToTable(folderPath, true);
        } else {
            mDbContext.songFolderManage.insertFolderToTable(folderPath, false);
        }
        auto children = folderPath.findChildFiles(juce::File::findFilesAndDirectories, false);
        // 因为可能出现子文件夹已经导入但是父文件夹又被用户重新导入了一遍，所以需要自己手写递归
        for (auto& child : children) {
            if (child.isDirectory()) {
                mFolderImportIsRoot = 1;
                importNewFolder(child);
            } else {
                checkAndInsertSong(child, false);
            }
        }
        mFolderImportIsRoot = 0;
    }
}