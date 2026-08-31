#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <atomic>
#include <string>

// 联网获取歌曲元数据
class OnlineGetMatedata : public juce::Thread {

public:
    struct Task {
        ///**
        // 发送任务情求的时候是否在导入文件的时候，因为导入文件的时候不做
        // 年份，作曲家，专辑艺术家，体裁，因为这四个基本上所有歌曲都没有
        // 只有在用在在后期请求再次联网搜索元数据的时候再执行
        //  */
        // bool isInput{true};

        double duration{0.0};
        std::string print; // 指纹

        bool needTitle{false};
        bool needArtist{false};
        bool needAlbum{false};
        // bool needTrackNumber{false};
        // bool needDiscNumber{false};
        bool needCover{false};

        // bool needYear{false};
        // bool needComposer{false};
        // bool needAlbumArtist{false};
        // bool needGenre{false};
    };

private:
    SQLite::Database& db;
    std::queue<Task> mTaskQueue;
    std::mutex mtx;

public:
    OnlineGetMatedata(SQLite::Database& db);
    ~OnlineGetMatedata() = default;
    void run() override;
    void setTask(Task task);

    void searchDataByPrint(Task task);
};