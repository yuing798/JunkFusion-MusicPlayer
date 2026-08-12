#pragma once

#include "constants.h"
#include "fileManage/songsManage.hpp"
#include "juce_core/juce_core.h"
#include "libExport.h"
#include <SQLiteCpp/Database.h>
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
    // JunkFusionBackendProcessManager mBackendMaster; // 后端进程管理

public:
    explicit dllManager();
    void init(const char* cacheDirId);

    juce::File& getSongImageDir() { return songImageDir; }

    static dllManager& getInstance() {
        static dllManager instance;
        return instance;
    } // 单例模式
    DONT_COPY_AND_MOVE(dllManager)

    // template <typename F> void runOnWrite(F&& fn) { // 万能引用
    //     // 不要把这个包装函数删除，因为以后想在入队前打印日志，改这一个函数就行
    //     writeWorker.addJob(std::forward<F>(fn));
    //     // std::forward<F>(fn) 的作用就是“按原样转发”：如果 F 推导为左值引用，它就返回左值；
    //     //  如果 F 推导为非引用（右值），它就返回右值（并附带移动语义）
    //     // std::forward 就是为了解决泛型编程中 “参数的左右值属性在传递过程中丢失” 的问题。
    //     //  它让 C++ 的模板代码既能保持极高的性能（减少拷贝），又能正确调用重载函数
    // }

    // template <typename F> void runOnRead(F&& fn) { readWorker.addJob(std::forward<F>(fn)); }

    SongsManage& getSongsManager() { return *songs; }
};