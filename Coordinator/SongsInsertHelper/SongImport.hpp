#pragma once

#include "Coordinator/CatagoryViewManage/songsManage.hpp"
#include "Coordinator/CenterDbManage/dbContext.hpp"
#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

enum class SongHasImportRes {
    HasImport,  // 歌曲已经导入
    NeedUpdate, // 歌曲已经导入但是需要更新，可能是因为用户使用了同名文件覆盖
    No,         // 完全没有，直接导入即可
};

// 这个类统一执行歌曲导入逻辑
class SongImport {
private:
    DbContext& mDbContext;
    juce::File& mImageDir;
    // juce::ThreadPool songImportPool{15};   // 用来做轻量歌曲元数据导入的线程池
    juce::ThreadPool waveFormAnalyPool{3}; // 用来做波形分析和联网搜索的线程池
    std::string mSystemCountry = juce::SystemStats::getUserRegion().toUpperCase().toStdString();

    struct WaveFormAnalyTask {
        std::string path;
        bool needGetPrint{false}; // 是否需要指纹进行联网搜索
    };

    /**
     * @brief
     *
     * @param dbManage
     * @param task 路径+是否需要指纹搜索
     * @return std::optional<std::string> 如果需要指纹搜索返回std::string否则返回std::nullopt
     */

    struct WaveFormAnalyRes {
        std::optional<std::string> print;
        std::array<double, 128> blobBuffer; // 波形图，只需要128根柱子
    };
    WaveFormAnalyRes waveFormAnaly(WaveFormAnalyTask task);

    struct OnlineSearchTask {

        int64_t songId{0};
        double duration{0.0};
        std::string print; // 指纹

        // 以下三个如果有的话发送过来，进行查找打分
        std::optional<std::string> title;
        std::optional<std::vector<std::string>> artists;
        std::optional<std::string> album;
        bool needCover{false};
    };

    static constexpr const char* AcoustIdKey = "lp1ajcs0Bd"; // 用来指纹搜索的AcoustId的密钥

    // 搜索专辑图片，因为可能需要分为多次搜索
    static std::optional<std::string> searchCoverURL(std::string term, std::string entity);

    struct OnlineSearchRes {
        std::optional<std::string> finalTitle;
        std::optional<std::vector<std::string>> finalArtists;
        std::optional<std::string> finalAlbum;
        std::optional<std::string> finalHash;
        std::optional<std::vector<std::string>> finalAlbumArtists;
    };

    OnlineSearchRes onlineSearch(OnlineSearchTask task);

    bool mFolderImportIsRoot{true}; // 给folderImport专用的:判断是否为根目录

    // SongsManage::insertSong的返回结果
    struct InsertLightSongInfoResult {

        SongInfo info;
        std::optional<std::vector<std::string>> albumArtists;
        bool titleIsFileName{false}; // title是否为文件名
    };

    std::optional<InsertLightSongInfoResult> insertLightSongInfo(
        const juce::File& path,
        bool isSingleFile,
        SongHasImportRes songHasImportRes
    );

public:
    explicit SongImport(DbContext& dbContext, juce::File& imageDir)
        : mDbContext(dbContext), mImageDir(imageDir) {}

    /**
     * @brief 检查并插入歌曲
     *
     * @param path 歌曲路径
     * @param isSingleFile 是单独导入还是随着文件夹导入
     * @return bool 插入成功与否
     */
    bool
    importSingleSong(SongHasImportRes updateOrNoInTable, const juce::File& path, bool isSingleFile);

    // 插入歌曲，这个函数只限于单独导入的文件
    void importSongs(Yvar songPaths);

    SongHasImportRes checkSongHasImport(const juce::File& path);

    std::function<void(const char*)> onAddOrUpdateSong;
    std::function<void(const std::string&)> onDirHasRegister;
};