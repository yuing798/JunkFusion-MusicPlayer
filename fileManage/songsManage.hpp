#pragma once

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/Transaction.h>
#include <cstdint>
#include <memory>
#include "songsModel.hpp"
#include "juce_core/juce_core.h"
#include "juce_gui_extra/juce_gui_extra.h"
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class SongsManage {
private:
    std::unique_ptr<SQLite::Database> songDb;
    std::unique_ptr<SQLite::Database> songImageDb;

    /** 使用 ICU Collator 按 title 排序后为所有歌曲重新分配 nameId */
    void rebuildNameIds();
    
public:

    /** 检查 filePath 是否已在数据库中（仅按路径匹配，不做 size/time 比较） */
    bool isSongExists(const std::string& filePath);
    SongsManage();
    ~SongsManage();

    /**
     * 插入一首歌，带事务保护。
     *
     * 防重复策略：
     *   1. 数据库层：filePath 有 UNIQUE 约束
     *   2. 代码层：插入前先查询 filePath，比较 fileSize 和 lastModifiedTime
     *      - 完全相同 → 视为重复，跳过插入
     *      - 不同 → 文件已更新，更新旧记录
     *      - 不存在 → 正常插入
     *
     * @param info     歌曲信息
     */
    bool insertSong(const SongInfo& info);

    /** 获取歌曲总数 */
    std::optional<int> getTotalSongCount();

    enum class SortMode {
        ByAddTime,
        ByName,
        ByPlayTimes
    };

    /** 分页获取歌曲，offset 从 0 开始，返回 limit 条记录，根据添加时间排序 */
    std::vector<SongInfo> getSongsPageBySongId(int offset, int limit, bool ascending);
    /** 分页获取歌曲，使用预先计算好的 nameId 进行 ICU 排序 */
    std::vector<SongInfo> getSongPageByName(int offset, int limit, bool ascending);

    std::vector<SongInfo> getSongPageByPlayTimes(int offset, int limit, bool ascending);
    /** 统一分页入口，根据 SortMode 选择排序方式 */
    std::vector<SongInfo> getSongPage(int offset, int limit, bool ascending, SortMode mode);

    static SongsManage& getInstance() {
        static SongsManage instance; //首次调用时创建，程序结束时自动析构
        return instance;
    }

    bool reverseMyLike(int64_t id);

    std::optional<std::string> getImageHashBySongId(int64_t songId);

    SongsManage(const SongsManage&) = delete;
    SongsManage& operator=(const SongsManage&) = delete;
    SongsManage(SongsManage&&) = delete;
    SongsManage& operator=(SongsManage&&) = delete;//强调全局唯一单例
};

class songsManageBuilder : public juce::OptionsBuilder<juce::WebBrowserComponent::Options>{
public:

    juce::WebBrowserComponent::Options buildOptions(const juce::WebBrowserComponent::Options& initial) override;
};