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

        bool needTitle{false};
        bool needArtist{false};
        bool needAlbum{false};
        bool needTrackNumber{false};
        bool needDiscNumber{false};
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
    void processSingleRequest(Task task);

    // 根据三个标签搜索元数据(三个标签必须同时存在才能调用这个函数)
    void searchDataByText(std::string title, std::string album, std::string artist);
    // void searchDataByPrint(std::string print);
};