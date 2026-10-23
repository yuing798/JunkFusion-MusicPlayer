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
    DLL_EXPORT void someImport(const char*);
    DLL_EXPORT void closeBackend(); // dll注销
    DLL_EXPORT void registerErrorSendCallback(StringFunc cb);

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
    DLL_EXPORT void registerOnPlayNextOrPreviousSong(IntFunc cb);     // 请求播放下一首或上一首歌曲
    DLL_EXPORT void registerOnLightSongDataImportOver(StringFunc cb); // 轻量歌曲数据导入完成的回调
    DLL_EXPORT void registerOnUpdateSongInfo(Int64Func cb);
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
    DLL_EXPORT const char* getAllPlaylistView();
    DLL_EXPORT const char* getDetailPlaylistInfo(long long id);
    DLL_EXPORT int createNewPlaylist(const char* request);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H