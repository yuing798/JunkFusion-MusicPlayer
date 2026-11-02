#ifndef LIB_EXPORT_H
#define LIB_EXPORT_H
#ifdef _WIN32
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __attribute__((visibility("default")))
#endif

typedef void (*StringFunc)(const char* str);
typedef void (*DoubleFunc)(double);
typedef void (*VoidFunc)();
typedef void (*Int64Func)(long long);
typedef void (*IntFunc)(int);

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief dll初始化
     *
     * @param cacheDirId 缓存目录
     * @param exeDirPtr exe文件的目录
     */
    DLL_EXPORT void dllInit(const char* cacheDirId, const char* exeDirPtr);

    DLL_EXPORT void toggleMyLike(long long songId);
    DLL_EXPORT void freeString(char* str);
    DLL_EXPORT void closeBackend(); // dll注销

    /**
     * @brief 注册「非致命错误」回调：前端收到后弹右下角卡片通知
     *
     * 消息体就是一段给人看的文字（比如 FFmpeg 解码失败的原因）。
     */
    DLL_EXPORT void registerErrorSendCallback(StringFunc cb);

    /**
     * @brief 注册「致命错误」回调：前端收到后弹中央模态窗，必须手动关掉
     *
     * 音频进程用 AudioMacro::fatalErrorPopupWindowMsg 这个 key 发出来的消息走这里；
     * 目前还没有生产者（解码失败都算非致命），通道先留着。
     */
    DLL_EXPORT void registerFatalErrorSendCallback(StringFunc cb);

    /**
     * @brief 播放
     *
     * @param songId 歌曲id
     * @param targetSeconds 目标秒数
     * @param isSeek 是否执行了跳转
     * @param isNewSong 是否为新歌
     * @return DLL_EXPORT
     */
    DLL_EXPORT void play(long long songId, double targetPTS, int isSeek, int isNewSong);
    DLL_EXPORT void pausePlay(); // 暂停播放
    DLL_EXPORT void registerCurrentPTSCallback(DoubleFunc doubleFunc);
    DLL_EXPORT void sendSliderValue(const char* identify, double value, int isOSC);
    DLL_EXPORT void registerOnPlayNextOrPreviousSong(IntFunc cb); // 请求播放下一首或上一首歌曲

    DLL_EXPORT const char* getAllSongs();
    DLL_EXPORT const char* getDetailSongInfo(long long songId);
    DLL_EXPORT const char* getBasicSongInfo(long long songId);
    DLL_EXPORT const char* getDetailAlbumInfo(long long albumId);
    DLL_EXPORT void requestOnPlayStateSync(IntFunc cb);

    /**
     * @brief Get the All Album View object
     *
     * @return DLL_EXPORT const* 所有专辑视图的json字符串
     */
    DLL_EXPORT const char* getAllAlbumView();

    // 歌单管理函数
    DLL_EXPORT const char* getDetailPlaylistInfo(long long id);
    DLL_EXPORT const char* getPlaylistBasicInfo(long long id);
    DLL_EXPORT const char* getAllPlaylistView();
    DLL_EXPORT long long createNewPlaylist(const char* title, const char* coverPath);
    DLL_EXPORT void addNewSongsToPlaylist(long long playlistId, const char* songIdsJson);
    DLL_EXPORT void removeSongsInPlaylist(long long playlistId, const char* songIdsJson);
    DLL_EXPORT void deletePlaylist(long long playlistId);
    DLL_EXPORT void renamePlaylist(long long playlistId, const char* title);
    DLL_EXPORT void updateCoverByPath(long long playlistId, const char* coverPath);
    DLL_EXPORT void updateCoverBySongId(long long playlistId, long long songId);

    //===================================================================================
    // 文件夹管理函数

    /**
     * @brief Get the Child Dir And Song Id List object
     *
     * @param parentDir
     * @return {songIdList:[id1,id2,......],dirArr:[subDir1,subDir2,......]}
     */
    DLL_EXPORT const char* getChildDirAndSongIdList(const char* parentDir);

    /**
     * @brief Get the Root Dir object获得在应用中注册过的所有文件夹根目录
     *
     * @return DLL_EXPORT const*
     */
    DLL_EXPORT const char* getAllRootDirView();
    /**
     * @brief 导入整个文件夹
     *
     * @param dir
     * @return DLL_EXPORT
     */
    DLL_EXPORT void importNewSongFolder(const char* dir);
    /**
     * @brief 导入单独的几个文件
     *
     * @param files
     * @return DLL_EXPORT
     */
    DLL_EXPORT void importSongs(const char* files);

    /**
     * @brief 获得单独导入的文件的songIdList，作为独立虚拟文件夹节点渲染在前端
     *
     * @return [songId1,songId2,......]
     */
    DLL_EXPORT const char* getSingleImportSongs();

    DLL_EXPORT void registerOnAddOrUpdateSong(StringFunc cb);
    DLL_EXPORT void registerOnDeleteSong(Int64Func cb);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H