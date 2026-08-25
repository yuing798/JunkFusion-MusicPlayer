#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <vector>

// 这个线程单独做插入歌曲的时候的波形图导入
class TimeDomainSpecInsert : public juce::Thread {
private:
    std::vector<std::string> mFileTasks;
    SQLite::Database& mDb;

public:
    TimeDomainSpecInsert(SQLite::Database& db);
    void run() override;
    void setTask(std::vector<std::string> files);
};