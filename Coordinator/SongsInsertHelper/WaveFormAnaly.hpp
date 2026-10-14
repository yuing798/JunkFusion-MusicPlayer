#pragma once

#include "./OnlineGetMatedata.hpp"
#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <vector>

// 这个线程单独做插入歌曲的时候的波形分析
class WaveFormAnaly {
public:
    struct Task {
        std::string path;
        // OnlineGetMatedata::Task onlineTask;
        bool needGetPrint{false}; // 是否需要指纹进行联网搜索
    };

    // private:
    //     CenterDbManage* dbManage;
    //     std::queue<Task> mTaskQueue;
    //     std::mutex mtx;
    //     OnlineGetMatedata mOnlineGetMatedata;

    // public:
    //     WaveFormAnaly(CenterDbManage* dbManage);
    //     ~WaveFormAnaly();
    //     void run() override;
    //     void setTask(Task task);

    /**
     * @brief
     *
     * @param dbManage
     * @param task 路径+是否需要指纹搜索
     * @return std::optional<std::string> 如果需要指纹搜索返回std::string否则返回std::nullopt
     */

    struct Result {
        std::optional<std::string> print;
        std::array<double, 128> blobBuffer; // 波形图，只需要128根柱子
    };
    static Result process(Task task);
};