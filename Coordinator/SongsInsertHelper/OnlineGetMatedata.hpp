#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

// 联网获取歌曲元数据
class OnlineGetMatedata {

public:
    struct Task {

        int64_t songId{0};
        double duration{0.0};
        std::string print; // 指纹

        // 以下三个如果有的话发送过来，进行查找打分
        std::optional<juce::String> title;
        std::optional<juce::StringArray> artists;
        std::optional<juce::String> album;
        bool needCover{false};
    };

private:
    // CenterDbManage* dbManage;
    // std::queue<Task> mTaskQueue;
    // std::mutex mtx;
    static constexpr const char* AcoustIdKey = "lp1ajcs0Bd"; // 用来指纹搜索的AcoustId的密钥
    // juce::String systemCountry;                              // 系统国家，用来专辑打分

    // 搜索专辑图片，因为可能需要分为多次搜索
    static std::optional<std::string> searchCoverURL(std::string term, std::string entity);

public:
    // OnlineGetMatedata(CenterDbManage* dbManage);
    // ~OnlineGetMatedata() = default;
    // void run() override;
    // void setTask(Task task);

    struct Result {
        std::optional<juce::String> finalTitle;
        std::optional<juce::StringArray> finalArtists;
        std::optional<juce::String> finalAlbum;
        std::optional<juce::String> finalHash;
        std::optional<juce::StringArray> finalAlbumArtists;
    };

    static Result process(juce::String systemCountry, Task task);
};