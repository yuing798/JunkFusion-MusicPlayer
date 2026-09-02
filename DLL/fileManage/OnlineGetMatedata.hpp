#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// 联网获取歌曲元数据
class OnlineGetMatedata : public juce::Thread {

public:
    struct Task {
        int64_t songId{0};

        double duration{0.0};
        std::string print; // 指纹

        // 以下三个如果有的话发送过来，进行查找打分
        std::optional<juce::String> title;
        std::vector<std::string> artists;
        std::optional<juce::String> album;
        bool needCover{false};
    };

private:
    SQLite::Database& db;
    std::queue<Task> mTaskQueue;
    std::mutex mtx;
    static constexpr const char* client_key = "lp1ajcs0Bd";

    // 辅助函数：判断是否为“群星/合集”类通配名字
    bool isVariousArtists(const std::string& name);

public:
    OnlineGetMatedata(SQLite::Database& db);
    ~OnlineGetMatedata() = default;
    void run() override;
    void setTask(Task task);

    void searchDataByPrint(Task task);
};