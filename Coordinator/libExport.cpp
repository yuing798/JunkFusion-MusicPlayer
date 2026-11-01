#include "./libExport.h"
#include "./dllManager.hpp"
#include "Coordinator/libExport.h"
#include "Macro/audioMacro.hpp"
#include "Macro/coordinatorMacro.hpp"
#include "Model/SongInfo.hpp"
#include "Model/playlistInfo.hpp"
#include "SongsInsertHelper/FolderCheckTimer.hpp"
#include "SongsInsertHelper/SongImport.hpp"
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
    const char* getChildDirAndSongIdList(const char* parentDir) {
        auto res = dllManager::getInstance().openNewSongFolder(juce::File{parentDir});

        return ConvertUtils::object2Uint8t(res);
    }
    const char* getAllRootDirView() {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance().getDbContext()->songFolderManage.getAllRootDirView()
        );
    }
    void importSongs(const char* files) {
        Yvar fileJson = Yvar::parse(files);
        // 单独导入：会先把音频文件拷到 <缓存目录>/songs 再入库
        dllManager::getInstance().getSongImportClass()->importSongs(fileJson);
    }
    void importNewSongFolder(const char* dir) {
        // 文件夹导入 + 定时监控都在 FolderCheckTimer 里
        dllManager::getInstance().getFolderCheckTimer()->importNewFolder(juce::File{dir});
    }
    const char* getSingleImportSongs() {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance().getDbContext()->songsManage.getSingleImportSongs()
        );
    }

    void registerOnAddOrUpdateSong(StringFunc cb) {
        dllManager::getInstance().getSongImportClass()->onAddOrUpdateSong = cb;
    }
    void registerOnDeleteSong(Int64Func cb) {
        dllManager::getInstance().getSongImportClass()->onDeleteSong = cb;
    }
}
