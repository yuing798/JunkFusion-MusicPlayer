#include "./dllManager.hpp"
#include "CenterDbManage/CenterDbmanage.hpp"
#include "CenterDbManage/dbContext.hpp"
#include "Coordinator/SongsManage/OnlineGetMatedata.hpp"
#include "Coordinator/SongsManage/songsManage.hpp"
#include "SongsManage/WaveFormAnaly.hpp"
#include "Utils/constants.h"
#include "Utils/dbUtils.hpp"
#include "juce_core/juce_core.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>
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
    songImageDir = cacheDir.getChildFile("songImage");
    if (!songImageDir.exists()) songImageDir.createDirectory();

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
#else
        dllLogger->set_level(spdlog::level::info);
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

    systemCountry = juce::SystemStats::getUserRegion().toUpperCase();
}

void dllManager::sendMessage2AudioProcess(juce::var& obj) {
    auto jsonStr{juce::JSON::toString(obj).toStdString()};
    mAudioProcessCoordinator->mAudioProcessPusher->sendMessage(jsonStr);
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
        // 检查并插入专辑
        if (!successRst.titleIsFileName && songInfo.album.has_value() &&
            songInfo.artists.has_value()) {
            juce::StringArray albumArtists;
            if (successRst.albumArtists.has_value()) {
                albumArtists = successRst.albumArtists.value();
            } else {
                albumArtists = songInfo.artists.value();
            }

            auto albumId = mDbContext->albumsManage.checkAlbumExist(
                songInfo.album.value(),
                albumArtists,
                songInfo.hash
            );
            if (!albumId.has_value()) {
                albumId = mDbContext->albumsManage
                              .insertAlbum(songInfo.album.value(), albumArtists, songInfo.hash);
            }
        }

        // 波形获取
        waveFormAnalyPool.addJob([successRst, this, path] {
            auto& songInfo{successRst.info};
            bool needGetPrint{false};
            if (successRst.titleIsFileName || !songInfo.artists.has_value() ||
                !songInfo.album.has_value() || !songInfo.hash.has_value()) {
                needGetPrint = true;
            }
            auto waveFormAnalyResult =
                WaveFormAnaly::process({path.getFullPathName().toStdString(), needGetPrint});
            OnlineGetMatedata::Result onlineGetMetadataResult;
            if (needGetPrint && waveFormAnalyResult.print.has_value()) {
                auto print{waveFormAnalyResult.print.value()};

                onlineGetMetadataResult =  OnlineGetMatedata::process(
                    systemCountry,
                    {
                        songInfo.songId,
                        songInfo.duration,
                        std::move(print),
                        successRst.titleIsFileName ? std::nullopt
                                                   : std::optional<juce::String>{songInfo.title},
                                                   songInfo.artists,
                                                   songInfo.album,
                                                   songInfo.hash.has_value() ? false:true

                    }
                );
            }
            try {

                mDbContext->dbManage.write([&](SQLite::Database& db) {
                    SQLite::Statement sql1{
                        db,
                        "UPDATE songs SET title = "
                        ":title,artists=:artists,album=:album,hash=:hash,timeDomainSpec=:"
                        "timeDomainSpec WHERE songId=:songId"
                    };
                    DbUtils::bindOptValue(sql1, ":title", onlineGetMetadataResult.finalTitle);
                    DbUtils::bindOptValue(sql1, ":artists", onlineGetMetadataResult.finalArtists);
                    DbUtils::bindOptValue(sql1, ":album", onlineGetMetadataResult.finalAlbum);
                    DbUtils::bindOptValue(sql1, ":hash", onlineGetMetadataResult.finalHash);
                    sql1.bind(
                        ":timeDomainSpec",
                        waveFormAnalyResult.blobBuffer.data(),
                        static_cast<int>(waveFormAnalyResult.blobBuffer.size() * sizeof(double))
                    );
                    sql1.bind(":songId", songInfo.songId);
                    sql1.exec();

                    auto albumId = mDbContext->albumsManage.getAlbumId(songInfo.songId);
                    if (albumId.has_value()) {
                        SQLite::Statement sql2{
                            db,
                            "UPDATE albums SET "
                            "title=:albumTitle,albumArtists=:albumArtists,hash=:hash WHERE "
                            "albumId=:albumId"
                        };
                        DbUtils::bindOptValue(
                            sql2,
                            ":albumTitle",
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
                    if (onUpdateSongInfo) onUpdateSongInfo(songInfo.songId);
                });
            } catch (SQLite::Exception& e) {
                spdlog::get(LogDllID)->error("dllManager::insertSong发生数据库错误:{}", e.what());
            }
        });

        return songInfo;
    }
    return std::string{};
}
