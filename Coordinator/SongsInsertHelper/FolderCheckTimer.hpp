#pragma once

#include "Coordinator/CenterDbManage/dbContext.hpp"
#include "SongImport.hpp"
#include "juce_core/juce_core.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

// ════════════════════════════════════════════════════════════════
// FolderCheckTimer —— 文件夹导入 + 文件夹状态定时监控
//
// 为什么不是 juce::Timer：
//   Coordinator 这个 DLL 里没有 MessageManager（只有 AudioProcessor 那边有
//   ScopedJuceInitialiser_GUI），而 juce::Timer 的回调是包装成 MessageBase 往
//   消息队列里 post 的：没有 MessageManager 时 post() 直接返回 false，回调
//   永远不会被调用。所以这里用一个自己的 juce::Thread，每分钟跑一轮。
//
// 一轮检查（checkOnce）分三段，前一段的出结果喂给下一段：
//   ① compareAllDirState()   只读：表里的目录还在不在、时间戳变没变；
//                            真实目录里表里没有的算新增
//   ② applyStructureChanges() 一个事务，只跑 SQL：删/增/刷新 songFolders
//   ③ syncDirContents()       事务外，对「新增」和「时间戳变了」的目录做文件级比对：
//                            盘有表无 → 导入；盘有表有但 mtime 变了 → 重导入；
//                            表有盘无 → 删行（并回调通知前端）
//
// 每一轮删歌之后还要收尾两件事（见 removeEmptyAlbums / onFolderStructureChanged）：
//   · 歌被删掉的专辑如果一首歌都不剩了，把 albums 那一行删掉并通知前端；
//   · 目录行有增删时通知前端重拉文件夹树。
//
// 注意：CenterDbManage::write 是「单线程池 + 阻塞等结果」，**不能嵌套调用**，
// 否则死锁。所以②里绝不允许出现 importSingleSong 之类的写库操作。
//
// 另外，Windows 上改文件内容不会改变父目录的 mtime，所以「原地改标签」这种
// 只能靠目录 mtime 之外的手段发现；现在的策略是只在目录时间戳变化时做文件
// 级比对（省 IO），原地改标签需要用户重新导入。
// ════════════════════════════════════════════════════════════════
class FolderCheckTimer : private juce::Thread {
public:
    FolderCheckTimer(DbContext& dbContext, SongImport& songImport);
    ~FolderCheckTimer() override;

    /// 启动监控线程（dllManager::init 里调用）
    void start();

    /// 停掉监控线程并 join（dllManager 析构时，必须在 SongImport/DbContext 之前）
    void stop();

    /// 立刻跑一轮检查（定时线程每轮跑的就是它）
    void checkOnce();

    /// 导入一个文件夹：前端 importNewSongFolder 走这里
    void importNewFolder(const juce::File& folderPath);

    /**
     * @brief 文件夹结构变了（有目录被删掉 / 新加进来）
     *
     * checkOnce 把结构变化落库之后触发。前端收到后重拉文件夹树 —— 不然用户在
     * 资源管理器里删掉一个已导入的文件夹，库里删干净了，前端那棵树却要重启才变。
     *
     * 只是「目录行」的增删；目录里歌曲的增删走 SongImport::onAddOrUpdateSong /
     * onDeleteSong。
     */
    std::function<void()> onFolderStructureChanged;

private:
    enum class SqlState {
        Update, // 这个文件夹时间戳变了
        Delete, // 这个文件夹被删除了
        Add,    // 这个文件夹是新添加的
    };
    struct DirState {
        SqlState sqlState{SqlState::Add};
        bool isRoot{false};
    };

    void run() override;

    /// 比较数据库里的文件夹和真实文件夹（只读，不写库）
    /// key 是目录的完整路径
    std::unordered_map<std::string, DirState> compareAllDirState();

    /// 把结构变化落库（一个事务里只跑 SQL），返回之后还要做内容比对的目录
    std::vector<std::string>
    applyStructureChanges(const std::unordered_map<std::string, DirState>& changes);

    /// 对单个目录做文件级比对（事务外，每首歌自己走一次 dbManage.write）
    void syncDirContents(const std::string& dirPath);

    /// 这些专辑里要是一首歌都不剩了，就把专辑行删掉并回调前端
    ///
    /// 必须在对应的歌**已经被删掉之后**调（函数自己会查 songs 表里还剩几首）。
    void removeEmptyAlbums(const std::vector<int64_t>& albumIds);

    DbContext& mDbContext;
    SongImport& mSongImport;

    /// 防止定时线程和前端手动导入同时扫同一棵树
    juce::CriticalSection mCheckLock;

    static constexpr int checkIntervalMs = 60 * 1000; // 一分钟一轮
};
