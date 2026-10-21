#pragma once

#include "CenterDbManage/CenterDbmanage.hpp"
#include "CenterDbManage/dbContext.hpp"
#include "Coordinator/SongsInsertHelper/OnlineGetMatedata.hpp"
#include "Model/SongInfo.hpp"
#include "Utils/constants.h"
#include "catagoryViewManage/songsManage.hpp"
#include "juce_core/juce_core.h"
#include "juce_osc/juce_osc.h"
#include "processSchedule/AudioProcessCoordinator.h"
#include "processSchedule/OscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <spdlog/sinks/rotating_file_sink.h>
#include <string>
#include <variant>

class dllManager {
private:
    juce::String systemCountry;

    juce::File cacheDir;   // 缓存文件夹
    juce::File logInfoDir; // 日志文件夹
    juce::File imageDir;   // 歌曲图片

    std::unique_ptr<OscSender> mOscSender; // 单例模式中的变量最好都是平凡类型的
    std::unique_ptr<AudioProcessCoordinator> mAudioProcessCoordinator; // zmq 版本的后端进程协调者

    static std::unique_ptr<dllManager> instance;

    std::unique_ptr<DbContext> mDbContext;
    juce::ThreadPool waveFormAnalyPool{5}; // 用来做波形分析和联网搜索的线程池

public:
    explicit dllManager();
    void init(const char* cacheDirId, const char* exeDirPtr);

    juce::File& getImageDir() { return imageDir; }

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

    std::function<void(const char*)> onErrorSendCallback; // cpp主动向前端发送错误数据
    std::function<void(double)> onCurrentPTSCallback;     // 获取当前的播放进度条的callback
    std::function<void(int)> onPlayNextOrPreviousSong;
    std::function<void(const char*)> onLightSongDataImportOver;
    std::function<void(int)> onPlayStateSync;      // 请求同步播放状态
    std::function<void(int64_t)> onUpdateSongInfo; // 更新播放信息

    void sendMessage2AudioProcess(juce::var& obj);
    void sendOSCMessage2AudioProcessor(juce::OSCMessage& msg) { mOscSender->sendMsg(msg); }

    std::variant<std::string, SongInfo> insertSong(const juce::File& path);

    DbContext* getDbContext() noexcept { return mDbContext.get(); }
};