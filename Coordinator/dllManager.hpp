#pragma once

#include "CenterDbManage/dbContext.hpp"
#include "SongsInsertHelper/SongImport.hpp"
#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include "juce_osc/juce_osc.h"
#include "processSchedule/AudioProcessCoordinator.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>

class FolderCheckTimer;

class dllManager {
private:
    juce::File cacheDir;   // 缓存文件夹
    juce::File logInfoDir; // 日志文件夹
    juce::File imageDir;   // 图片
    juce::File songsDir;   // 单独导入的歌曲拷贝（和 JunkFusion.db 同一层）

    std::unique_ptr<OscSender> mOscSender; // 单例模式中的变量最好都是平凡类型的
    std::unique_ptr<AudioProcessCoordinator> mAudioProcessCoordinator; // zmq 版本的后端进程协调者

    static std::unique_ptr<dllManager> instance;

    std::unique_ptr<DbContext> mDbContext;
    std::unique_ptr<SongImport> mSongImport;
    std::unique_ptr<FolderCheckTimer> mFolderCheckTimer; // 文件夹定时监控（独立线程）

public:
    // cpp主动向前端发送错误数据：非致命 -> 前端弹右下角卡片；致命 -> 前端弹模态窗
    std::function<void(const char*)> onErrorSendCallback;
    std::function<void(const char*)> onFatalErrorSendCallback;
    std::function<void(double)> onCurrentPTSCallback; // 获取当前的播放进度条的callback
    std::function<void(int)> onPlayNextOrPreviousSong;
    std::function<void(int)> onPlayStateSync; // 请求同步播放状态

    /**
     * @brief 库里某首歌的元数据变了（我喜欢 / 播放次数）
     *
     * 查一次这首歌最新的 basic 信息，转成 SongImport::onAddOrUpdateSong 推给前端
     * （前端 SongProvider.addOrUpdateSong 更新那个 id -> 信息 的缓存）。
     * 以前这里是 onUpdateSongInfo(int64) 回调，只传 id，现在统一走 add/update 那条路。
     */
    void notifySongUpdated(int64_t songId);

    void sendMessage2AudioProcess(const std::string& obj);
    void sendOSCMessage2AudioProcessor(juce::OSCMessage& msg) { mOscSender->sendMsg(msg); }

    Yvar openNewSongFolder(const juce::File& path); // 打开新的歌曲文件夹

    DbContext* getDbContext() noexcept { return mDbContext.get(); }
    SongImport* getSongImportClass() noexcept { return mSongImport.get(); }
    FolderCheckTimer* getFolderCheckTimer() noexcept { return mFolderCheckTimer.get(); }

    explicit dllManager();
    void init(const char* cacheDirId, const char* exeDirPtr);

    juce::File& getImageDir() { return imageDir; }
    juce::File& getSongsDir() { return songsDir; }

    static dllManager& getInstance() {
        if (!instance) {
            instance.reset(new dllManager()); // 第一次调用时创建
        }
        return *instance;
    }

    static void destroyInstance() {
        if (instance) {
            instance.reset();
        }
    }

    DONT_COPY_AND_MOVE(dllManager)

    ~dllManager();
};