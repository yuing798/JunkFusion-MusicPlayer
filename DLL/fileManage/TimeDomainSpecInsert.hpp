#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

// 这个线程单独做插入歌曲的时候的波形图导入
class TimeDomainSpecInsert : public juce::Thread {
private:
    SQLite::Database& mDb;
    std::queue<std::string> mTaskQueue;
    std::mutex mtx;

public:
    TimeDomainSpecInsert(SQLite::Database& db);
    ~TimeDomainSpecInsert();
    void run() override;
    void setTask(std::string file);
    void processSingleFile(std::string file);
    std::function<void(const char*)> onFileTaskOver; // 任务完成时通知前端
};