#pragma once

#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Coordinator/CenterDbManage/dbContext.hpp"
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

enum class SongHasImportRes {
    HasImport,  // 歌曲已经导入
    NeedUpdate, // 歌曲已经导入但是需要更新，可能是因为用户使用了同名文件覆盖
    No,         // 完全没有，直接导入即可
};

// 这个类统一执行歌曲导入逻辑
//
// ── 两种导入方式，落库的 filePath 不一样 ──────────────────────────
// 1. 随文件夹导入（isSingleFile = false）：filePath = 用户磁盘上的原路径，
//    文件留在原处不拷贝。文件夹视图靠这个路径做「目录里的文件 → songId」匹配。
// 2. 单独导入（isSingleFile = true）：先把音频文件**拷贝**到
//    <缓存目录>/songs 下，filePath 记的是拷贝出来的那份。
//    文件名 = <原路径的 SHA1>.<原扩展名>（见 getCachedFileFor）：
//      · 是确定的，所以重复导入同一份文件能算回同一个路径，用它判重/覆盖，
//        不需要再把原路径存进数据库；
//      · 拷贝是先写 .tmp 再改名，中途失败不会留下半个文件；
//      · 拷贝失败直接放弃这一首的导入（不会留下指向不存在文件的记录）。
class SongImport {
private:
    DbContext& mDbContext;
    juce::File& mImageDir; // 封面缓存目录：<缓存目录>/image/<hash>/original.jpg
    juce::File& mSongsDir; // 单独导入的歌曲拷贝目录：<缓存目录>/songs

    // TODO(AI): 僵尸函数，目前**没有任何调用点**。
    //
    // 时域图已经从产品里删掉了，但这里保留了完整的解码 + 重采样流程
    // （统一成 11025Hz / 单声道 / S16P），以后接 AI 音频分析时直接把
    // 分析逻辑插进解码循环即可。
    struct WaveFormAnalyTask {
        std::string path;
    };
    void waveFormAnaly(WaveFormAnalyTask task);

    // SongsManage::insertSong的返回结果
    struct InsertLightSongInfoResult {

        SongInfo info;
        std::optional<std::vector<std::string>> albumArtists;
        bool titleIsFileName{false}; // title是否为文件名

        /// 这个歌曲最终落库、并且应该拿去播放/分析的路径：
        /// 文件夹歌曲 = 原路径；单独导入的歌曲 = 缓存里的拷贝
        juce::File finalPath;

        /// 这次是否真的拷了一份到缓存（NeedUpdate 且不需要重拷时为 false）
        bool copiedToCache{false};
    };

    std::optional<InsertLightSongInfoResult> insertLightSongInfo(
        const juce::File& path,
        bool isSingleFile,
        SongHasImportRes songHasImportRes
    );

    /**
     * @brief 单独导入的歌曲在缓存里的目标文件（不负责拷贝，只算路径）
     *
     * 文件名 = <origin 完整路径的 SHA1>.<origin 的扩展名>，放在 mSongsDir 下。
     * 路径是确定的，所以「先算路径 → 查库判重 → 再决定要不要拷」这条路才成立。
     *
     * @param origin 用户选的原始音频文件
     * @return 缓存目标文件（此时可能还不存在）
     */
    juce::File getCachedFileFor(const juce::File& origin) const;

public:
    explicit SongImport(DbContext& dbContext, juce::File& imageDir, juce::File& songsDir)
        : mDbContext(dbContext), mImageDir(imageDir), mSongsDir(songsDir) {}

    /**
     * @brief 检查并插入歌曲
     *
     * @param updateOrNoInTable 先调 checkSongHasImport 拿到结果再传进来
     * @param path 歌曲路径（原文件路径）
     * @param isSingleFile true = 单独导入（会拷贝进缓存），false = 随文件夹导入
     * @return bool 插入成功与否
     */
    bool
    importSingleSong(SongHasImportRes updateOrNoInTable, const juce::File& path, bool isSingleFile);

    // 插入歌曲，这个函数只限于单独导入的文件
    void importSongs(Yvar songPaths);

    /**
     * @brief 这个文件在库里吗、变过没有
     *
     * @param path 原文件路径（对单独导入来说，是用户选的那份文件，不是缓存副本）
     * @param isSingleFile true 时按「算出来的缓存路径」查库，false 时按 filePath 查
     */
    SongHasImportRes checkSongHasImport(const juce::File& path, bool isSingleFile);

    // 每导入/更新一首歌回调一次，参数是这首歌的 basic json（toBasicJson）
    std::function<void(const char*)> onAddOrUpdateSong;

    // 每从库里删掉一首歌回调一次（文件夹里的文件被删掉时会发生）
    std::function<void(int64_t)> onDeleteSong;

    /**
     * @brief 新建了一张专辑时回调一次
     *
     * 参数是这张专辑的 basic json，结构和 getAllAlbumsView 里的一项完全一样，
     * 前端直接塞进专辑列表就行。已经有同名专辑时（checkAlbumExist 命中）
     * 不会回调。
     */
    std::function<void(const char*)> onAddNewAlbum;

    /**
     * @brief 专辑里一首歌都不剩、被删掉时回调一次
     *
     * 参数是 albumId。删专辑的动作在 FolderCheckTimer::removeEmptyAlbums 里做
     * （见 AlbumsManage::removeAlbumIfEmpty），这里只负责通知前端。
     */
    std::function<void(int64_t)> onDeleteAlbum;

    std::function<void(const std::string&)> onDirHasRegister;
};
