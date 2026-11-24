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
    DLL_EXPORT void registerOnPlayNextOrPreviousSong(IntFunc cb); // 请求播放下一首或上一首歌

    /**
     * @brief 注册「音频进程被自动重启了」回调
     *
     * 插件崩溃/挂死会把音频进程带走，Coordinator 的看门狗发现后会重新拉起它。
     * 重启后音频进程里什么都没有（当前歌曲、进度全丢），前端收到必须：
     *   · 把播放状态切回"停止"；
     *   · 提示用户"音频进程异常，已自动重启（第 N 次）"。
     *
     * @param cb 参数是第几次重启（从 1 开始）；**-1 = 反复崩溃，看门狗已放弃**，
     *           这时要让用户手动处理（别再自动点了）。
     *
     * TODO(契约): 真正的插件加载还没做，所以现在重启后不会自动恢复任何插件
     *            （host-integration.md §6.2：绝不能自动恢复上次崩溃的插件）。
     */
    DLL_EXPORT void registerOnAudioProcessRestarted(IntFunc cb);

    DLL_EXPORT const char* getDetailSongInfo(long long songId);
    DLL_EXPORT const char* getBasicSongInfo(long long songId);

    /**
     * @brief 取某首歌的歌词（已解析并合并翻译）
     *
     * @return {"lyric":{"hasTranslation":bool,"lines":[{"startMs","text","translation","words"}]}}
     *         没有歌词时 {"lyric":null}
     */
    DLL_EXPORT const char* getSongLyrics(long long songId);

    /**
     * @brief 得到详细的专辑信息，现在详细专辑信息里面只有歌曲列表，以后可能加上用户标签之类的
     *
     * @param albumId
     * @return DLL_EXPORT const*
     */
    DLL_EXPORT const char* getDetailAlbumInfo(long long albumId);
    DLL_EXPORT void requestOnPlayStateSync(IntFunc cb);

    /**
     * @brief Get the All Album View object
     *
     * @return DLL_EXPORT const* 所有专辑视图的json字符串
     */
    DLL_EXPORT const char* getAllAlbumView();

    /**
     * @brief 联网搜索某张专辑的封面并写进 albums.hash
     *
     * 导入阶段不再联网，封面靠这个按需补（前端右键「联网搜索专辑封面」）。
     * 搜索词是「主艺术家 + 专辑名」，群星这类名字会被跳过，搜不到再用纯专辑名兜底。
     *
     * @return {"hash":"..."} 成功（前端拿它刷新这一张专辑）；{"hash":null} 失败
     */
    DLL_EXPORT const char* searchAlbumCover(long long albumId);

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

    /**
     * @brief 固定歌单的歌曲（我喜欢 / 最近播放 / 最多播放）
     *
     * 这三个**不是** playlists 表里的行，而是对 songs 表的三种固定查询：
     *   liked       → isMyLike=1，按最近播放时间倒序，**不限条数**
     *   recent      → 有 lastPlayTime，按时间倒序，最多 200 首
     *   mostPlayed  → playNum>0，按次数倒序，最多 200 首
     * 所以歌单页把它们当"虚拟歌单"渲染，右键没有删除菜单。
     *
     * @param kind "liked" / "recent" / "mostPlayed"；其它值返回空数组
     * @return {"songs":[...]}，元素结构和 getBasicSongInfo 完全一致
     */
    DLL_EXPORT const char* getFixedPlaylistSongs(const char* kind);

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
    // 实时频谱（共享内存，音频进程写 / 这里读）

    /** @brief 频段数量（固定 96），前端按它分配数组 */
    DLL_EXPORT int getSpectrumBandCount();

    /**
     * @brief 取当前频谱一帧（0~1 归一化的 96 个对数频段）
     *
     * 数据来自音频进程写的共享内存，按 seqlock 读快照。
     * 返回的指针指向内部缓冲，**下一次调用会被覆盖**，请立刻拷走。
     *
     * @return 96 个 float；当前没有数据（未初始化 / 没在播放 / 已关闭）时返回 nullptr
     */
    DLL_EXPORT const float* getSpectrumFrame();

    /** @brief 取峰值保持值（同样 96 个 float，生命周期同上） */
    DLL_EXPORT const float* getSpectrumPeaks();

    /**
     * @brief 打开/关闭音频进程的频谱分析
     *
     * 播放页打开时传 1，离开时传 0；关掉能省掉 FFT 开销。
     */
    DLL_EXPORT void setSpectrumEnabled(int enabled);

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

    //===================================================================================
    // 插件库（VST3 扫描）
    //
    // 扫描 = 遍历 pluginDirs 里的 .vst3 → 在**临时子进程**里枚举元数据 → 写 plugins 表。
    // 子进程是为了隔离坏插件：坏插件在枚举阶段就能把进程带走，绝不能让它带走音频进程。
    // 扫描是后台跑的，前端调 startPluginScan() 之后轮询 getPluginScanState()。

    /**
     * @brief 开始一次扫描（后台线程，立即返回）
     *
     * @return {"started":true|false, "error":"..."}
     *         started=false 只有两种情况：已经在扫 / 插件模块没初始化。
     */
    DLL_EXPORT const char* startPluginScan();

    /**
     * @brief 扫描状态（前端用来画"扫描中 5/12"）
     *
     * @return {"scan":{"state":"idle"|"running"|"done"|"failed",
     *                  "total":int,"processed":int,"failed":int,"message":"..."}}
     */
    DLL_EXPORT const char* getPluginScanState();

    /**
     * @brief 插件列表
     *
     * @return {"plugins":[{"filePath","identifier","name","vendor","category","type",
     *                      "numInputs","numOutputs","scanStatus","scanError"}]}
     *         type 只有 "instrument" / "effect" / "other" 三种；
     *         scanStatus=="failed" 的行 name 是文件名，scanError 里是原因。
     */
    DLL_EXPORT const char* getAllPluginsView();

    /** @brief 扫描目录列表 → {"dirs":[{"path":"...","isBuiltin":bool}]} */
    DLL_EXPORT const char* getPluginDirs();

    /** @brief 加一个扫描目录 → {"ok":bool,"error":"..."} */
    DLL_EXPORT const char* addPluginDir(const char* path);

    /** @brief 删一个扫描目录 → {"ok":bool,"error":"..."}（系统默认目录会被拒绝） */
    DLL_EXPORT const char* removePluginDir(const char* path);

    /**
     * @brief 手动改插件类型（前端唯一允许改的字段）
     *
     * 重扫**不会**覆盖这里改的值——元数据整条错掉的插件（OB-Xd 报 0/0）
     * 就靠这个纠正。失败时返回 {"ok":false}（identifier 不存在 / type 非法）。
     */
    DLL_EXPORT const char* setPluginType(const char* identifier, const char* type);

    //===================================================================================
    // 全局效果器链（8 个插槽：交叉淡化之后、EQ 之前，见 docs/audio-routing.md）
    //
    // "当前链"的权威副本在 Coordinator；音频进程负责实例化 + 处理 + 回报状态。

    /**
     * @brief 当前链的加载状态（音频进程回传）
     *
     * @return {"chain":{"bypassAll":bool,"anyLoaded":bool,"latency":int,
     *                  "slots":[{"index","loaded","bypass","name","identifier",
     *                            "filePath","latency","error"}]}}
     *         还没收到过状态时 chain 为 null
     */
    DLL_EXPORT const char* getPluginChainState();

    /**
     * @brief 下发整链配置（前端编辑完点"应用"）
     *
     * @param slotsJson {"bypassAll":bool,"slots":[{"index":0,"filePath":"...",
     *                  "identifier":"...","bypass":false,"state":"<base64>"}]}
     * @return {"ok":bool,"error":"...","dropped":["identifier"...]}
     *         dropped = 被黑名单挡掉、没有下发的插件（白名单可以放行）
     */
    DLL_EXPORT const char* setPluginChain(const char* slotsJson);

    /** @brief 整链旁通开关（1 = 旁通整条链，声音回到干声） */
    DLL_EXPORT void setPluginChainBypassAll(int bypass);

    /** @brief 单个插槽旁通开关（slot = 0~7） */
    DLL_EXPORT void setPluginChainSlotBypass(int slot, int bypass);

    /**
     * @brief 打开某个插槽的插件编辑器窗口
     *
     * 窗口出现在**音频进程**里（那边有消息循环）；同一个槽再点一次 = 把窗口叫回前台。
     * 换链/音频进程重启后窗口会消失。
     */
    DLL_EXPORT void openPluginSlotEditor(int slot);

    /** @brief 链预设列表 → {"presets":[{id,title,createTime}]} */
    DLL_EXPORT const char* getAllPluginChainPresets();

    /** @brief 某个预设的配置体 → {"chain":{...}}；找不到时 chain 为 null */
    DLL_EXPORT const char* getPluginChainPreset(long long id);

    /** @brief 把一份配置存成新预设（前端把当前 8 槽 JSON 传上来）→ 新 id，失败 -1 */
    DLL_EXPORT long long savePluginChainPreset(const char* title, const char* slotsJson);

    DLL_EXPORT void renamePluginChainPreset(long long id, const char* title);
    DLL_EXPORT void deletePluginChainPreset(long long id);

    //===================================================================================
    // 插件黑白名单（host-integration.md §6）
    //
    // 两张表分开是刻意的：黑名单是系统观测，白名单是用户意图，后者优先级最高。

    /** @brief {"blacklist":[{identifier,filePath,reason,failureCount,lastFailedAt}],
     *          "whitelist":[{identifier,filePath,note,createdAt}]} */
    DLL_EXPORT const char* getPluginPolicyView();

    /** @brief 强制允许某个插件（用户意图）→ {"ok":bool,"error":""} */
    DLL_EXPORT const char* addPluginWhitelist(const char* identifier, const char* filePath);

    DLL_EXPORT const char* removePluginWhitelist(const char* identifier);

    /** @brief 解除某个插件的黑名单（"重试"） */
    DLL_EXPORT const char* removePluginBlacklist(const char* identifier);

#ifdef __cplusplus
}
#endif

#endif // LLB_EXPORT_H