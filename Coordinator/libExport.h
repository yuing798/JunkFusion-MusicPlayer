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

    /**
     * @brief 注册「文件夹结构变了」回调：前端收到后重拉文件夹树
     *
     * 用户在资源管理器里删掉 / 新增了已经导入过的文件夹时触发（定时检查那一轮，
     * 或者 refreshForntend 手动催的那一轮）。只是某个目录里歌曲的增删不会走
     * 这里，那个走 registerOnAddOrUpdateSong / registerOnDeleteSong。
     */
    DLL_EXPORT void registerOnFolderStructureChanged(VoidFunc cb);

    /**
     * @brief 注册「新建了一张专辑」回调
     *
     * 参数是这张专辑的 basic json，和 getAllAlbumView 里的一项长得一样。
     */
    DLL_EXPORT void registerOnAddNewAlbum(StringFunc cb);

    /**
     * @brief 注册「专辑被删掉了」回调
     *
     * 参数是 albumId。专辑里一首歌都不剩时后端会把 albums 那一行删掉再回调。
     */
    DLL_EXPORT void registerOnDeleteAlbum(Int64Func cb);

    DLL_EXPORT void refreshForntend(); // 刷新前端，立刻扫描一次文件夹所有歌曲信息

    //===================================================================================
    // EQ 均衡器
    //
    // 参数分两条通道下发（都通过 Utils/ParamLink，带全局单调 rev）：
    //   拖拽中  -> OSC/UDP，可能丢帧
    //   松手等  -> TCP JSON，必达、权威
    // 所以**每次拖拽结束都必须再调一次 setEqBand 把最终值定死**。

    /**
     * @brief 拖拽中的连续参数（高频，走 OSC/UDP）
     *
     * 只改 freqHz / gainDb / q / shelfSlope 四个连续量，类型和开关不变。
     */
    DLL_EXPORT void
        updateEqBandLive(int idx, double freqHz, double gainDb, double q, double shelfSlope);

    /**
     * @brief 一条 band 的完整参数（换类型、开关、拖拽结束的最终值），走 TCP JSON
     *
     * @param idx            0 ~ 7
     * @param enabled        0/1
     * @param type           "bell" / "lowPass" / "highPass" / "notch" / "bandPass"
     *                       / "allPass" / "lowShelf" / "highShelf" / "tile" / "flatTile"
     *                       （认不出来的按 bell 处理）
     * @param cutSlopeDbPerOct LowPass/HighPass 用：12 = 一段，24 = 两段级联
     */
    DLL_EXPORT void setEqBand(
        int idx,
        int enabled,
        const char* type,
        double freqHz,
        double gainDb,
        double q,
        double shelfSlope,
        int cutSlopeDbPerOct
    );

    /** @brief 整机开关 + 输出增益 */
    DLL_EXPORT void setEqMaster(int enabled, double masterGainDb);

    /**
     * @brief 整机参数（预设 / 初始化）
     *
     * @param eqAllJson {"eqAll":{...}}，也可以直接传内层的 {...}
     */
    DLL_EXPORT void setEqAll(const char* eqAllJson);

    /** @brief 取当前整机参数，返回 {"eqAll":{...}}，可以直接喂回 setEqAll */
    DLL_EXPORT const char* getEqAllState();

    /**
     * @brief 取整机频响曲线（dB 值数组）
     *
     * 频率轴：freq(i) = 20 * (20000/20)^(i / (numPoints-1))，i = 0 .. numPoints-1
     * 返回的指针指向内部缓冲，**下一次调用会被覆盖**，请拿到后立刻拷贝。
     *
     * @param numPoints 采样点数（会被钳到 2~2048）
     * @return 长度 numPoints 的 float 数组；未初始化时返回 nullptr
     */
    DLL_EXPORT const float* getEqCurve(int numPoints);

    //===================================================================================
    // EQ 预设（存在 eqPresets 表里）

    /** @brief 所有预设的列表，返回 {"eqPresets":[{id,title,createTime}]}（不含参数体） */
    DLL_EXPORT const char* getAllEqPresets();

    /**
     * @brief 取某个预设的参数，返回 {"eqAll":{...}}，可以直接喂给 setEqAll()
     *
     * 找不到这个 id 时内层是空值（"eqAll": null），前端按"预设不存在"处理。
     */
    DLL_EXPORT const char* getEqPresetParams(long long id);

    /**
     * @brief 把「当前 EQ 状态」存成一个新预设
     *
     * @return 新预设的 id；失败返回 -1
     */
    DLL_EXPORT long long saveEqPreset(const char* title);

    DLL_EXPORT void renameEqPreset(long long id, const char* title);
    DLL_EXPORT void deleteEqPreset(long long id);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H