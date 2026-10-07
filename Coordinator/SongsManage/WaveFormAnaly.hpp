#pragma once

#include "./OnlineGetMatedata.hpp"
#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
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
    CenterDbManage* dbManage;
    std::queue<Task> mTaskQueue;
    std::mutex mtx;
    OnlineGetMatedata mOnlineGetMatedata;

public:
    WaveFormAnaly(CenterDbManage* dbManage);
    ~WaveFormAnaly();
    void run() override;
    void setTask(Task task);
    void processSingleFile(Task task);
};