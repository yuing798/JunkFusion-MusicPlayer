#include "./libExport.h"
#include "./dllManager.hpp"
#include "Coordinator/libExport.h"
#include "Macro/PlaylistMacro.hpp"
#include "Macro/SongInfoMacro.hpp"
#include "Macro/audioMacro.hpp"
#include "Macro/coordinatorMacro.hpp"
#include "Model/SongInfo.hpp"
#include "Model/playlistInfo.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "Utils/convertUtils.hpp"
#include "Utils/otherUtils.hpp"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_osc/juce_osc.h"
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <variant>
#include <vector>

extern "C" {
    void dllInit(const char* cacheDirId, const char* exeDirPtr) {
        OtherUtils::writeEmergencyLog("准备初始化dll单例");
        dllManager::getInstance().init(cacheDirId, exeDirPtr);
    }
    void toggleMyLike(long long songId) {
        dllManager::getInstance().getDbContext()->songsManage.reverseMyLike(songId);
    }
    void freeString(char* str) {
        if (str) {
            free(str);
        }
    }
    void someImport(const char* jsonStr) {
        auto jsonString = std::string(jsonStr);
        juce::Thread::launch([jsonStr = std::move(jsonString)]() {
            auto logger{spdlog::get(LogDllID)};
            logger->info("开始导入歌曲");
            logger->debug("jsonStr == {}", jsonStr);
            Yvar obj = Yvar::parse(jsonStr);
            auto filePaths{obj.read(CoordinatorMacro::filePaths)};
            std::vector<juce::File> paths;
            for (int i = 0; i < filePaths.size(); i++) {
                paths.push_back(juce::File{filePaths.read(i).toString()});
            }
            Yvar errorFiles = Yvar::array();
            Yvar songInfos = Yvar::array();
            for (auto& path : paths) {
                auto result = dllManager::getInstance().insertSong(path);
                if (std::holds_alternative<std::string>(result)) {
                    auto errorMsg = std::get<std::string>(result);
                    Yvar errorObj = Yvar::object();
                    errorObj.set(CoordinatorMacro::errorFileName, path.getFileName().toStdString());
                    errorObj.set(CoordinatorMacro::errorFileReason, errorMsg);
                    errorFiles.push(std::move(errorObj));
                } else if (std::holds_alternative<SongInfo>(result)) {
                    auto songInfo = std::get<SongInfo>(result);
                    songInfos.push(songInfo.toBasicJson());
                }
            }

            Yvar resultObj = Yvar::object();
            {
                resultObj.set(CoordinatorMacro::errorFiles, errorFiles);

                std::string resultStr{
                    "导入歌曲完成，成功" + std::to_string(paths.size()) + "首，失败" +
                    std::to_string(errorFiles.size()) + "首\n失败文件：\n" + errorFiles.dump()
                };
                logger->info(resultStr);
            }
            resultObj.set(CoordinatorMacro::songsList, songInfos);

            // 发送最终数据
            if (dllManager::getInstance().onLightSongDataImportOver)
                dllManager::getInstance().onLightSongDataImportOver(
                    ConvertUtils::object2Uint8t(resultObj)
                );
        });
        return;
    }
    void closeBackend() {
        spdlog::get(LogDllID)->debug("准备关闭后端");
        dllManager::destroyInstance();
    }

    void registerErrorSendCallback(StringFunc cb) {
        dllManager::getInstance().onErrorSendCallback = cb;
    }
    void play(long long songId, double targetSeconds, int isSeek, int isNewSong) {
        auto info =
            dllManager::getInstance().getDbContext()->songsManage.getPlayInfo(songId).toJson();
        Yvar playObj = Yvar::object();
        {
            playObj.set(AudioMacro::playInfo, info);
            playObj.set(AudioMacro::targetPTS, targetSeconds);
            playObj.set(AudioMacro::isSeek, isSeek == 0 ? false : true);
            playObj.set(AudioMacro::isNewSong, isNewSong == 0 ? false : true);
        }
        Yvar resultObj = Yvar::object();
        resultObj.set(AudioMacro::play, playObj);
        dllManager::getInstance().sendMessage2AudioProcess(resultObj.dump());
    }
    void pausePlay() {
        Yvar obj = Yvar::object();
        obj.set(AudioMacro::pause, "");
        dllManager::getInstance().sendMessage2AudioProcess(obj.dump());
    }
    void registerCurrentPTSCallback(DoubleFunc doubleFunc) {
        dllManager::getInstance().onCurrentPTSCallback = doubleFunc;
    }
    void sendSliderValue(const char* identify, double value, int isOSC) {
        auto identifyParam = identify;
        if (isOSC) {
            juce::OSCMessage msg(identifyParam, static_cast<float>(value));
            dllManager::getInstance().sendOSCMessage2AudioProcessor(msg);
        } else {
            Yvar obj = Yvar::object();

            Yvar paramAndValueObj = Yvar::object();
            {
                paramAndValueObj.set(AudioMacro::sliderParam, identifyParam);
                paramAndValueObj.set(AudioMacro::sliderValue, value);
            }
            {
                obj.set(AudioMacro::setSliderValue, paramAndValueObj);
            }
            dllManager::getInstance().sendMessage2AudioProcess(obj.dump());
        }
    }
    void registerOnPlayNextOrPreviousSong(IntFunc cb) {
        dllManager::getInstance().onPlayNextOrPreviousSong = cb;
    }

    void registerOnLightSongDataImportOver(StringFunc cb) {
        dllManager::getInstance().onLightSongDataImportOver = cb;
    }
    void registerOnUpdateSongInfo(Int64Func cb) { dllManager::getInstance().onUpdateSongInfo = cb; }
    const char* getAllSongs() {
        auto songsList =
            dllManager::getInstance().getDbContext()->songsManage.getAllSongsBasicInfo();
        Yvar obj = Yvar::object();
        obj.set(CoordinatorMacro::songsList, std::move(songsList));

        return ConvertUtils::object2Uint8t(obj);
    }
    const char* getBasicSongInfo(long long songId) {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance()
                .getDbContext()
                ->songsManage.getBasicSongInfo(songId)
                .toBasicJson()
        );
    }

    const char* getDetailSongInfo(long long songId) {
        auto info{dllManager::getInstance().getDbContext()->songsManage.getDetailSongInfo(songId)};
        return ConvertUtils::object2Uint8t(info.toDetailJson());
    }
    void requestOnPlayStateSync(IntFunc cb) { dllManager::getInstance().onPlayStateSync = cb; }

    const char* getAllAlbumView() {
        Yvar obj = Yvar::object();
        obj.set(
            CoordinatorMacro::albumsList,
            dllManager::getInstance().getDbContext()->albumsManage.getAllAlbumsView()
        );
        return ConvertUtils::object2Uint8t(obj);
    }

    const char* getDetailAlbumInfo(long long albumId) {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance().getDbContext()->albumsManage.getDetailInfo(albumId)
        );
    }

    const char* getAllPlaylistView() {
        Yvar obj = Yvar::object();
        obj.set(
            CoordinatorMacro::playlistsList,
            dllManager::getInstance().getDbContext()->playListManage.getAllPlayListView()
        );
        return ConvertUtils::object2Uint8t(obj);
    }
    const char* getDetailPlaylistInfo(long long id) {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance()
                .getDbContext()
                ->playListManage.getDetailInfo(id)
                .toDetailJson()
        );
    }
    const char* getPlaylistBasicInfo(long long id) {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance().getDbContext()->playListManage.getBasicInfo(id).toBasicJson()
        );
    }
    long long createNewPlaylist(const char* title, const char* coverPath) {

        return dllManager::getInstance().getDbContext()->playListManage.createNewPlaylist(
            title,
            !std::string{coverPath}.empty() ? std::optional<juce::File>{juce::File{coverPath}}
                                            : std::nullopt
        );
    }
    void addNewSongsToPlaylist(long long playlistId, const char* songIdsJson) {
        std::vector<int64_t> songIds = Yvar::parse(songIdsJson).toVector<int64_t>();
        dllManager::getInstance().getDbContext()->playListManage.addNewSongs(playlistId, songIds);
    }
    void removeSongsInPlaylist(long long playlistId, const char* songIdsJson) {
        std::vector<int64_t> songIds = Yvar::parse(songIdsJson).toVector<int64_t>();
        dllManager::getInstance().getDbContext()->playListManage.removeSongs(playlistId, songIds);
    }
    void deletePlaylist(long long playlistId) {
        dllManager::getInstance().getDbContext()->playListManage.deletePlaylist(playlistId);
    }
    void renamePlaylist(long long playlistId, const char* title) {
        dllManager::getInstance().getDbContext()->playListManage.renamePlayList(playlistId, title);
    }
    void updateCoverByPath(long long playlistId, const char* coverPath) {
        auto hash =
            dllManager::getInstance().getDbContext()->playListManage.coverPathCopyAndConvertToHash(
                juce::File{coverPath}
            );
        dllManager::getInstance().getDbContext()->playListManage.updateCover(playlistId, hash);
    }
    void updateCoverBySongId(long long playlistId, long long songId) {
        dllManager::getInstance().getDbContext()->playListManage.updateCover(playlistId, songId);
    }
}
