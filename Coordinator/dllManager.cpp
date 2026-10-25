#include "./dllManager.hpp"
#include "CenterDbManage/CenterDbmanage.hpp"
#include "CenterDbManage/dbContext.hpp"
#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Coordinator/SongsInsertHelper/OnlineGetMatedata.hpp"
#include "SongsInsertHelper/WaveFormAnaly.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include "juce_core/juce_core.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include <optional>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <variant>

std::unique_ptr<dllManager> dllManager::instance = nullptr;

dllManager::dllManager() {}

void dllManager::init(const char* cacheDirId, const char* exeDirPtr) {

    cacheDir = juce::File{cacheDirId};
    logInfoDir = cacheDir.getChildFile("log");
    if (!logInfoDir.exists()) logInfoDir.createDirectory();
    imageDir = cacheDir.getChildFile("image");
    if (!imageDir.exists()) imageDir.createDirectory();

    constexpr size_t kUiMaxSize = 5 * 1024 * 1024; // 5 MB
    constexpr size_t kUiMaxFiles = 3;

    {
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logInfoDir.getChildFile("dll.log").getFullPathName().toStdString(),
            kUiMaxSize,
            kUiMaxFiles
        );
        // 全局日志格式：时间戳 + 级别 + 线程ID + 消息体
        const char* pattern = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v";
        auto dllLogger = std::make_shared<spdlog::logger>(LogDllID, std::move(sink));
        dllLogger->set_pattern(pattern);
#ifdef JF_DEBUG
        dllLogger->set_level(spdlog::level::debug);
        dllLogger->flush_on(spdlog::level::debug);
#else
        dllLogger->set_level(spdlog::level::info);
        dllLogger->flush_on(spdlog::level::err);
#endif

        spdlog::register_logger(dllLogger);
    }

    auto logger{spdlog::get(LogDllID)};
    logger->debug("准备初始化音频进程");

    mAudioProcessCoordinator = std::make_unique<AudioProcessCoordinator>();
    mOscSender = std::make_unique<OscSender>();
    if (!mAudioProcessCoordinator->start(
            juce::File{exeDirPtr}.getChildFile("JunkFusionAudioProcess.exe"),
            mOscSender->getPort(),
            juce::File{cacheDirId}
        )) {
        logger->critical("音频进程启动或握手失败");
    }
    logger->debug("已通知音频进程启动");

    juce::File dbFile{cacheDir.getChildFile("JunkFusion.db")};

    mDbContext = std::make_unique<DbContext>(dbFile.getFullPathName().toStdString());

    systemCountry = juce::SystemStats::getUserRegion().toUpperCase().toStdString();
}

void dllManager::sendMessage2AudioProcess(const std::string& obj) {
    mAudioProcessCoordinator->mAudioProcessPusher->sendMessage(obj);
}

dllManager::~dllManager() {
    spdlog::get(LogDllID)->debug("---------------------------------------");
    spdlog::shutdown();
}
std::variant<std::string, SongInfo> dllManager::insertSong(const juce::File& path) {
    auto result = mDbContext->songsManage.insertSong(path);
    if (std::holds_alternative<std::string>(result)) {
        return std::get<std::string>(result);
    } else if (std::holds_alternative<SongsManage::InsertSongResult>(result)) {
        auto successRst{std::get<SongsManage::InsertSongResult>(result)};
        auto& songInfo{successRst.info};
        // spdlog::get(LogDllID)->debug(
        //     "songsManage.insertSong的结果为:{}",
        //     songInfo.toDetailJson().toString()
        // );
        // return songInfo;

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
            albumId = mDbContext->albumsManage.checkAlbumExist(
                songInfo.album.value(),
                albumArtists.value(),
                songInfo.hash
            );
            if (!albumId.has_value()) {
                albumId = mDbContext->albumsManage.insertAlbum(
                    songInfo.album.value(),
                    albumArtists.value(),
                    songInfo.hash
                );
            }

            mDbContext->dbManage.write([&](SQLite::Database& db) {
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
                WaveFormAnaly::process({path.getFullPathName().toStdString(), needSearchSongInfo});

            OnlineGetMatedata::Result onlineGetMetadataResult;
            if (needSearchSongInfo && waveFormAnalyResult.print.has_value()) {
                auto print{waveFormAnalyResult.print.value()};

                // 专辑艺术家如果没有不需要联网搜索，因为如果要搜索专辑艺术家的话每首歌都需要这么做
                onlineGetMetadataResult =  OnlineGetMatedata::process(
                    systemCountry,
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

                mDbContext->dbManage.write([&](SQLite::Database& db) {
                    SQLite::Transaction t{db};
                    SQLite::Statement sql1{
                        db,
                        "UPDATE songs SET "
                        "title=COALESCE(:title,title),artists=COALESCE(:artists,artists),album="
                        "COALESCE(:album,album),hash=COALESCE(:hash,hash),timeDomainSpec=:"
                        "timeDomainSpec WHERE songId=:songId"
                    };

                    if (needSearchSongTitle) {
                        DbUtils::bindOptValue(sql1, ":title", onlineGetMetadataResult.finalTitle);
                    }
                    if (needSearchArtists) {
                        DbUtils::bindOptValue(
                            sql1,
                            ":artists",
                            onlineGetMetadataResult.finalArtists
                        );
                    }

                    if (needSearchAlbum) {
                        DbUtils::bindOptValue(sql1, ":album", onlineGetMetadataResult.finalAlbum);
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
                        auto albumId = mDbContext->albumsManage.getAlbumId(songInfo.songId);
                        if (albumId.has_value()) {

                            SQLite::Statement sql2{
                                db,
                                "UPDATE albums SET "
                                "title=COALESCE(:title,title),albumArtists=COALESCE(:albumArtists,"
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
                            DbUtils::bindOptValue(sql2, ":hash", onlineGetMetadataResult.finalHash);
                            DbUtils::bindOptValue(sql2, ":albumId", albumId);
                            sql2.exec();
                        }
                    }
                    t.commit();
                });
                if (onUpdateSongInfo) onUpdateSongInfo(songInfo.songId);
            } catch (SQLite::Exception& e) {
                spdlog::get(LogDllID)->error("dllManager::insertSong发生数据库错误:{}", e.what());
            }
        });

        return songInfo;
    }
    return std::string{};
}
