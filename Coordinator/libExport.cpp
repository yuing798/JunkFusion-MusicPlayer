#include "./libExport.h"
#include "./EqController.hpp"
#include "./SpectrumController.hpp"
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
    void registerFatalErrorSendCallback(StringFunc cb) {
        dllManager::getInstance().onFatalErrorSendCallback = cb;
    }
    void play(long long songId, double targetSeconds, int isSeek, int isNewSong) {
        auto* dbContext = dllManager::getInstance().getDbContext();

        // 收到「播放新歌」信号就记一下最近播放时间（暂停后继续播、拖动进度条都不算）
        if (isNewSong != 0 && dbContext != nullptr) {
            dbContext->songsManage.updateLastPlayTime(songId);
        }

        auto info = dbContext->songsManage.getPlayInfo(songId).toJson();
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
    void registerOnAudioProcessRestarted(IntFunc cb) {
        dllManager::getInstance().onAudioProcessRestarted = cb;
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

    const char* getBasicSongInfo(long long songId) {
        return ConvertUtils::object2Uint8t(
            dllManager::getInstance()
                .getDbContext()
                ->songsManage.getBasicSongInfo(songId)
                .toBasicJson()
        );
    }

    const char* getSongLyrics(long long songId) {
        Yvar envelope = Yvar::object();

        // FFI 边界不能让任何异常逃出去：C++ 异常穿过 FFI 会直接终止进程
        try {
            auto* dbContext = dllManager::getInstance().getDbContext();
            if (dbContext != nullptr) {
                auto document = dbContext->lyricsManage.get(songId);
                envelope.set("lyric", document.has_value() ? document->toJson() : Yvar{});
            } else {
                envelope.set("lyric", Yvar{});
            }
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("getSongLyrics 异常 songId={}:{}", songId, e.what());
            envelope.set("lyric", Yvar{});
        } catch (...) {
            envelope.set("lyric", Yvar{});
        }

        return ConvertUtils::object2Uint8t(envelope);
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

    const char* searchAlbumCover(long long albumId) {
        auto& manager = dllManager::getInstance();
        Yvar result = Yvar::object();

        auto* dbContext = manager.getDbContext();
        if (dbContext == nullptr) {
            result.set("hash", Yvar{});
            return ConvertUtils::object2Uint8t(result);
        }

        auto hash = dbContext->albumsManage.searchAndSaveCover(albumId, manager.getImageDir());
        result.set("hash", Yvar::fromOptional(hash));
        return ConvertUtils::object2Uint8t(result);
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
    void registerOnFolderStructureChanged(VoidFunc cb) {
        auto* folderCheckTimer = dllManager::getInstance().getFolderCheckTimer();
        if (!folderCheckTimer) return;
        folderCheckTimer->onFolderStructureChanged = cb;
    }
    void registerOnAddNewAlbum(StringFunc cb) {
        dllManager::getInstance().getSongImportClass()->onAddNewAlbum = cb;
    }
    void registerOnDeleteAlbum(Int64Func cb) {
        dllManager::getInstance().getSongImportClass()->onDeleteAlbum = cb;
    }
    void refreshForntend() { dllManager::getInstance().getFolderCheckTimer()->checkOnce(); }

    //===================================================================================
    // EQ 均衡器
    void updateEqBandLive(int idx, double freqHz, double gainDb, double q, double shelfSlope) {
        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr) return;

        eqController->updateBandLive(
            idx,
            static_cast<float>(freqHz),
            static_cast<float>(gainDb),
            static_cast<float>(q),
            static_cast<float>(shelfSlope)
        );
    }

    void setEqBand(
        int idx,
        int enabled,
        const char* type,
        double freqHz,
        double gainDb,
        double q,
        double shelfSlope,
        int cutSlopeDbPerOct
    ) {
        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr) return;

        EqBandParam band{};
        band.idx = idx;
        band.enabled = enabled != 0;
        if (type != nullptr) {
            // 认不出来的类型保持默认（bell），不要静默改成别的形状
            if (auto parsed = EqBandParam::typeFromName(type)) band.type = *parsed;
        }
        band.freqHz = static_cast<float>(freqHz);
        band.gainDb = static_cast<float>(gainDb);
        band.q = static_cast<float>(q);
        band.shelfSlope = static_cast<float>(shelfSlope);
        band.cutSlopeDbPerOct = cutSlopeDbPerOct > 0 ? cutSlopeDbPerOct : 12;

        eqController->setBand(band);
    }

    void setEqMaster(int enabled, double masterGainDb) {
        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr) return;

        eqController->setMaster(enabled != 0, static_cast<float>(masterGainDb));
    }

    void setEqAll(const char* eqAllJson) {
        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr || eqAllJson == nullptr) return;

        const Yvar envelope = Yvar::parse(eqAllJson);
        Yvar body = envelope.read("eqAll");
        if (!body.isObject()) body = envelope; // 允许直接传内层的参数体

        eqController->setAll(EqWire::decodeAll(body));
    }

    const char* getEqAllState() {
        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr) return ConvertUtils::object2Uint8t(Yvar::object());

        Yvar envelope = Yvar::object();
        envelope.set("eqAll", EqWire::encodeAll(eqController->getState()));
        return ConvertUtils::object2Uint8t(envelope);
    }

    const float* getEqCurve(int numPoints) {
        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr) return nullptr;
        return eqController->getCurve(numPoints);
    }

    //===================================================================================
    // 实时频谱
    int getSpectrumBandCount() { return SpectrumController::bandCount(); }

    const float* getSpectrumFrame() {
        auto* spectrumController = dllManager::getInstance().getSpectrumController();
        if (spectrumController == nullptr) return nullptr;
        return spectrumController->getBands();
    }

    const float* getSpectrumPeaks() {
        auto* spectrumController = dllManager::getInstance().getSpectrumController();
        if (spectrumController == nullptr) return nullptr;
        return spectrumController->getPeaks();
    }

    void setSpectrumEnabled(int enabled) {
        auto* spectrumController = dllManager::getInstance().getSpectrumController();
        if (spectrumController == nullptr) return;
        spectrumController->setEnabled(enabled != 0);
    }

    //===================================================================================
    // EQ 预设
    const char* getAllEqPresets() {
        Yvar envelope = Yvar::object();
        envelope.set(
            "eqPresets",
            dllManager::getInstance().getDbContext()->eqPresetManage.getAllPresets()
        );
        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* getEqPresetParams(long long id) {
        Yvar envelope = Yvar::object();
        envelope.set(
            "eqAll",
            dllManager::getInstance().getDbContext()->eqPresetManage.getPresetParams(id)
        );
        return ConvertUtils::object2Uint8t(envelope);
    }

    long long saveEqPreset(const char* title) {
        if (title == nullptr) return -1;

        auto* eqController = dllManager::getInstance().getEqController();
        if (eqController == nullptr) return -1;

        const std::string paramsText = EqWire::encodeAll(eqController->getState()).dump();
        return dllManager::getInstance().getDbContext()->eqPresetManage.createPreset(
            title,
            paramsText
        );
    }

    void renameEqPreset(long long id, const char* title) {
        if (title == nullptr) return;
        dllManager::getInstance().getDbContext()->eqPresetManage.renamePreset(id, title);
    }

    void deleteEqPreset(long long id) {
        dllManager::getInstance().getDbContext()->eqPresetManage.deletePreset(id);
    }

    //===================================================================================
    // 固定歌单（我喜欢 / 最近播放 / 最多播放）
    //
    // 它们不是 playlists 表里的行，是三种固定查询，所以歌单页把三者当虚拟歌单渲染。
    // 返回完整的歌曲基本信息（不是 songId 列表），前端可以直接拿去播/显示，
    // 不用再按 id 逐个回查。
    const char* getFixedPlaylistSongs(const char* kind) {
        Yvar envelope = Yvar::object();
        Yvar songs = Yvar::array();

        // FFI 边界不能让异常逃出去
        try {
            auto* dbContext = dllManager::getInstance().getDbContext();
            if (dbContext != nullptr && kind != nullptr) {
                songs = dbContext->songsManage.getFixedPlaylistSongs(kind);
            }
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("getFixedPlaylistSongs 异常:{}", e.what());
        } catch (...) {
        }

        envelope.set("songs", songs);
        return ConvertUtils::object2Uint8t(envelope);
    }

    //===================================================================================
    // 插件库（VST3 扫描）
    //
    // 这一层只做三件事：拿指针、调 PluginHost、把结果包成 JSON。
    // 所有可能抛异常的地方都兜住——C++ 异常穿过 FFI 会直接把 Flutter 带走。
    //
    // 注意这里没有抽"插件模块未初始化"的公共小函数：AGENTS.md 禁止在 cpp 顶层
    // 放散装工具函数，而这点重复（三行）不值得为它单开一个类。

    const char* startPluginScan() {
        Yvar envelope = Yvar::object();

        try {
            auto* job = dllManager::getInstance().getPluginScanJob();
            if (job == nullptr) {
                envelope.set("ok", false);
                envelope.set("started", false);
                envelope.set("error", "插件模块未初始化");
                return ConvertUtils::object2Uint8t(envelope);
            }

            const bool started = job->startScan();
            envelope.set("ok", started);
            envelope.set("started", started);
            envelope.set("error", started ? "" : "扫描已在进行中");
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("startPluginScan 异常:{}", e.what());
            envelope.set("ok", false);
            envelope.set("started", false);
            envelope.set("error", "启动扫描失败");
        } catch (...) {
            envelope.set("ok", false);
            envelope.set("started", false);
            envelope.set("error", "启动扫描失败");
        }

        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* getPluginScanState() {
        Yvar envelope = Yvar::object();

        try {
            auto* job = dllManager::getInstance().getPluginScanJob();
            envelope.set("scan", job != nullptr ? Yvar::parse(job->toJson()) : Yvar{});
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("getPluginScanState 异常:{}", e.what());
            envelope.set("scan", Yvar{});
        } catch (...) {
            envelope.set("scan", Yvar{});
        }

        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* getAllPluginsView() {
        Yvar envelope = Yvar::object();
        Yvar plugins = Yvar::array();

        try {
            auto* catalog = dllManager::getInstance().getPluginCatalog();
            if (catalog != nullptr) {
                for (const auto& row : catalog->getAll()) {
                    Yvar item = Yvar::object();
                    item.set("filePath", row.filePath);
                    item.set("identifier", row.identifier);
                    item.set("name", row.name);
                    item.set("vendor", row.vendor);
                    item.set("category", row.category);
                    item.set("type", row.type);
                    item.set("numInputs", row.numInputs);
                    item.set("numOutputs", row.numOutputs);
                    item.set("scanStatus", row.scanStatus);
                    item.set("scanError", row.scanError);
                    plugins.push(item);
                }
            }
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("getAllPluginsView 异常:{}", e.what());
        } catch (...) {
        }

        envelope.set("plugins", plugins);
        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* getPluginDirs() {
        Yvar envelope = Yvar::object();
        Yvar dirs = Yvar::array();

        try {
            auto* catalog = dllManager::getInstance().getPluginCatalog();
            if (catalog != nullptr) {
                for (const auto& row : catalog->getDirs()) {
                    Yvar item = Yvar::object();
                    item.set("path", row.path);
                    item.set("isBuiltin", row.isBuiltin);
                    dirs.push(item);
                }
            }
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("getPluginDirs 异常:{}", e.what());
        } catch (...) {
        }

        envelope.set("dirs", dirs);
        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* addPluginDir(const char* path) {
        Yvar envelope = Yvar::object();

        try {
            auto* catalog = dllManager::getInstance().getPluginCatalog();
            if (catalog == nullptr || path == nullptr) {
                envelope.set("ok", false);
                envelope.set("error", "插件模块未初始化");
                return ConvertUtils::object2Uint8t(envelope);
            }

            const auto error = catalog->addDir(path);
            envelope.set("ok", error.empty());
            envelope.set("error", error);
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("addPluginDir 异常:{}", e.what());
            envelope.set("ok", false);
            envelope.set("error", "添加失败");
        }

        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* removePluginDir(const char* path) {
        Yvar envelope = Yvar::object();

        try {
            auto* catalog = dllManager::getInstance().getPluginCatalog();
            if (catalog == nullptr || path == nullptr) {
                envelope.set("ok", false);
                envelope.set("error", "插件模块未初始化");
                return ConvertUtils::object2Uint8t(envelope);
            }

            const auto error = catalog->removeDir(path);
            envelope.set("ok", error.empty());
            envelope.set("error", error);
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("removePluginDir 异常:{}", e.what());
            envelope.set("ok", false);
            envelope.set("error", "删除失败");
        }

        return ConvertUtils::object2Uint8t(envelope);
    }

    const char* setPluginType(const char* identifier, const char* type) {
        Yvar envelope = Yvar::object();

        try {
            auto* catalog = dllManager::getInstance().getPluginCatalog();
            if (catalog == nullptr || identifier == nullptr || type == nullptr) {
                envelope.set("ok", false);
                envelope.set("error", "插件模块未初始化");
                return ConvertUtils::object2Uint8t(envelope);
            }

            const bool ok = catalog->setType(identifier, type);
            envelope.set("ok", ok);
            envelope.set("error", ok ? "" : "identifier 不存在或 type 非法");
        } catch (const std::exception& e) {
            if (auto log = spdlog::get(LogDllID))
                log->error("setPluginType 异常:{}", e.what());
            envelope.set("ok", false);
            envelope.set("error", "修改失败");
        }

        return ConvertUtils::object2Uint8t(envelope);
    }
}
