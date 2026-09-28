#pragma once

#include "./OnlineGetMatedata.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

// 这个线程单独做插入歌曲的时候的波形分析
class WaveFormAnaly : public juce::Thread {
public:
    struct Task {
        std::string path;
        OnlineGetMatedata::Task onlineTask;
    };

private:
    SQLite::Database& mDb;
    std::queue<Task> mTaskQueue;
    std::mutex mtx;
    OnlineGetMatedata mOnlineGetMatedata;

public:
    WaveFormAnaly(SQLite::Database& db);
    ~WaveFormAnaly();
    void run() override;
    void setTask(Task task);
    void processSingleFile(Task task);
    std::function<void(const char*)> onOnlineGetMatedataOver;
};