/**
 * @brief 全局效果器链的 Coordinator 侧：持有当前配置、按名单过滤、下发、缓存音频回传的状态
 *
 * ── 它管什么 ──────────────────────────────────────────────────────────
 *   1. "当前链"的 8 槽配置（**权威副本在 Coordinator**，前端改完通过 FFI 推上来）；
 *   2. 下发前按黑白名单过滤：黑名单里的插件默认不下发（白名单可以强制放行）；
 *   3. 缓存音频进程回传的 `pluginChainState`（每个槽加载成功没有、延迟多少）。
 *
 * ── 它不管什么 ────────────────────────────────────────────────────────
 *   · 不碰数据库（预设是 PluginChainPresetManage 的事）；
 *   · 不实例化插件（那是音频进程的事，本类只发 JSON）。
 *
 * ── 线程 ─────────────────────────────────────────────────────────────
 *   setBypassAll / setSlotBypass / apply 来自 FFI（Dart 线程）；
 *   onStateFromAudio 来自 ZMQ SUB 线程。内部加锁。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     chainController->init(sendLambda);          // dllManager::init 里一次
 *     chainController->apply(configYvar);         // 应用一份 8 槽配置
 *     chainController->setBypassAll(true);
 *     chainController->setSlotBypass(0, true);
 *     chainController->getState();                // 给前端轮询
 */
#pragma once

#include "PluginHost/PluginPreflightRunner.hpp"
#include "Utils/Yvar.hpp"

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

class PluginChainController {
public:
    static constexpr int kSlotCount = 8;

    void init(const std::function<void(const std::string&)>& send);

    /** @brief 注入扫描子进程（预检用）；不存在时预检会被跳过并记警告 */
    void setScanExe(juce::File scanExe);

    /**
     * @brief 下发一份链配置
     *
     * @param config {"bypassAll":bool,"slots":[{index,filePath,identifier,bypass,state}]}
     * @return {"ok":bool,"error":"...","dropped":[identifier...]}（被名单挡掉的插件）
     */
    Yvar apply(const Yvar& config);

    void setBypassAll(bool bypassAll);
    void setSlotBypass(int slot, bool bypass);

    /**
     * @brief 打开某个槽的插件编辑器窗口
     *
     * 窗口开在**音频进程**里（那边有消息循环，插件实例也在那边）。
     * 音频进程重启后窗口就没了，再点一次即可。
     */
    void openEditor(int slot);

    /** @brief 音频进程回传的状态（SUB 线程调） */
    void onStateFromAudio(const Yvar& state);

    /** @brief {"chain":{...}}；没收到过状态时 chain 为空 */
    Yvar getState();

    /**
     * @brief 音频进程重启后调用：清空缓存的加载状态
     *
     * **不会**自动重发链配置——host-integration.md §6.2 要求不能自动恢复
     * 上次崩溃的插件，让用户自己再点一次。
     */
    void resetAfterRestart();

    /** @brief 当前配置的副本（前端刷新时用） */
    Yvar getConfig();

private:
    /// 按黑白名单过滤 slots（黑名单且非白名单 → 丢弃），被丢弃的 identifier 进 dropped
    Yvar filterByPolicy(const Yvar& config, std::vector<std::string>& dropped);

    /**
     * @brief 没预检过就先预检一次（同一个插件本次会话只跑一次）
     *
     * 预检在临时子进程里跑（PluginPreflightRunner），崩了/挂了只影响子进程。
     * 采样率/块大小用 44100/441（真实设备值；延迟跟块大小有关）。
     */
    bool runPreflightIfNeeded(
        const std::string& filePath,
        const std::string& identifier,
        std::string& reason
    );

    /// 音频进程在"刚下发完、还没回状态"这段时间里重启 → 判定为运行期崩溃，拉黑
    void banPendingAsRuntimeCrash();

    std::function<void(const std::string&)> mSend;

    mutable std::mutex mMutex;
    Yvar mConfig;
    Yvar mState;

    /// 预检器（没有扫描子进程时为空 → 跳过预检）
    std::unique_ptr<PluginHost::PluginPreflightRunner> mPreflightRunner;

    /// "identifier|filePath" -> 预检是否通过（本次会话缓存，避免每次应用都重跑）
    std::map<std::string, bool> mPreflightCache;

    /// 刚下发、还没等到音频进程回报状态的插件（用来判"运行期崩溃"）
    std::set<std::string> mPendingIdentifiers;
};
