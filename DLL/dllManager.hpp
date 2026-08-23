#pragma once

#include "constants.h"
#include "fileManage/songsManage.hpp"
#include "juce_core/juce_core.h"
#include "libExport.h"
#include "processManager/AudioProcessCoordinator.h"
#include "processManager/oscSender.hpp"
#include <SQLiteCpp/Database.h>
#include <fstream>
#include <functional>
#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <utility>

class dllManager {
private:
    std::unique_ptr<SQLite::Database> db;
    std::unique_ptr<SongsManage> songs; // 歌曲管理
    juce::File cacheDir;                // 缓存文件夹
    juce::File logInfoDir;              // 日志文件夹
    juce::File songImageDir;            // 歌曲图片

    std::unique_ptr<OscSender> mOscSender; // 单例模式中的变量最好都是平凡类型的
    std::unique_ptr<AudioProcessCoordinator> mAudioProcessCoordinator; // zmq 版本的后端进程协调者

    static std::unique_ptr<dllManager> instance;

public:
    explicit dllManager();
    void init(const char* cacheDirId, const char* exeDirPtr);

    juce::File& getSongImageDir() { return songImageDir; }

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

    SongsManage& getSongsManager() { return *songs; }

    ~dllManager();

    std::function<void(const char*)> errorSendCallback;
    void sendMessage2AudioProcess(std::string msg) {
        mAudioProcessCoordinator->mAudioProcessPusher->sendMessage(msg);
    }
};