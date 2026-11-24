#include "./PluginChainController.hpp"

#include "./dllManager.hpp"
#include "Macro/audioMacro.hpp"
#include "Utils/constants.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

void PluginChainController::init(const std::function<void(const std::string&)>& send) {
    mSend = send;
}

void PluginChainController::setScanExe(juce::File scanExe) {
    if (!scanExe.existsAsFile()) {
        if (auto logger = spdlog::get(LogDllID))
            logger->warn(
                "[Preflight] 没找到扫描子进程，插件将不做预检: {}",
                scanExe.getFullPathName().toStdString()
            );
        return;
    }

    mPreflightRunner = std::make_unique<PluginHost::PluginPreflightRunner>(std::move(scanExe));
}

bool PluginChainController::runPreflightIfNeeded(
    const std::string& filePath,
    const std::string& identifier,
    std::string& reason
) {
    const std::string key = identifier + "|" + filePath;

    {
        std::lock_guard<std::mutex> lock{mMutex};
        const auto it = mPreflightCache.find(key);
        if (it != mPreflightCache.end()) return it->second;
    }

    if (mPreflightRunner == nullptr) {
        // 没有预检器（开发环境没打包 scan exe）：放行，不因此拒绝用户
        return true;
    }

    // 采样率/块大小用真实设备值（44100/441）：延迟值跟块大小有关，
    // 实验台 512 块下 CHOWTapeModel 报 39，441 下报 37
    const auto result = mPreflightRunner->run(
        juce::File{juce::String::fromUTF8(filePath.c_str())},
        identifier,
        44100.0,
        441,
        10000
    );

    {
        std::lock_guard<std::mutex> lock{mMutex};
        mPreflightCache[key] = result.ok;
    }

    if (!result.ok) reason = result.error;
    return result.ok;
}

Yvar PluginChainController::filterByPolicy(
    const Yvar& config,
    std::vector<std::string>& dropped
) {
    Yvar out = Yvar::object();
    out.set("bypassAll", config.read("bypassAll").toBool());

    auto* dbContext = dllManager::getInstance().getDbContext();
    PluginPolicyManage* policy = dbContext != nullptr ? &dbContext->pluginPolicyManage : nullptr;

    Yvar slots = Yvar::array();
    const auto input = config.read("slots");
    for (int i = 0; i < input.size(); ++i) {
        const auto item = input.read(i);
        const std::string identifier = item.read("identifier").toString();
        const std::string filePath = item.read("filePath").toString();

        if (policy != nullptr && !identifier.empty()) {
            // 1) 已经在黑名单里（且不在白名单）→ 直接不下发
            if (policy->isBlocked(identifier)) {
                dropped.push_back(identifier);
                if (auto logger = spdlog::get(LogDllID))
                    logger->warn("[PluginChain] 跳过黑名单插件: {}", identifier);
                continue;
            }

            // 2) 没预检过的，先在一个临时子进程里走一遍完整生命周期
            std::string reason;
            if (!runPreflightIfNeeded(filePath, identifier, reason)) {
                // 预检失败 = 确定性复现，直接拉黑 + 不下发
                policy->ban(identifier, filePath, "preflight-failed");
                dropped.push_back(identifier);
                if (auto logger = spdlog::get(LogDllID))
                    logger->error("[PluginChain] 预检失败已拉黑 {}: {}", identifier, reason);
                continue;
            }
        }

        slots.push(item);
    }

    out.set("slots", slots);
    return out;
}

Yvar PluginChainController::apply(const Yvar& config) {
    Yvar result = Yvar::object();

    if (!mSend) {
        result.set("ok", false);
        result.set("error", "音频进程未连接");
        result.set("dropped", Yvar::array());
        return result;
    }

    std::vector<std::string> dropped;
    Yvar filtered = filterByPolicy(config, dropped);

    {
        std::lock_guard<std::mutex> lock{mMutex};
        mConfig = filtered;
        // 换链之后旧状态就作废了，等音频进程回传新的
        mState = Yvar{};
    }

    Yvar envelope = Yvar::object();
    envelope.set(AudioMacro::pluginChain, filtered);
    mSend(envelope.dump());

    // 记下"这次下发包含哪些插件"：如果音频进程在回报状态之前就崩了，
    // 说明是它们中的某一个把进程带走的（resetAfterRestart 里会拉黑）
    {
        std::lock_guard<std::mutex> lock{mMutex};
        mPendingIdentifiers.clear();
        const auto sentSlots = filtered.read("slots");
        for (int i = 0; i < sentSlots.size(); ++i) {
            const std::string identifier = sentSlots.read(i).read("identifier").toString();
            if (!identifier.empty()) mPendingIdentifiers.insert(identifier);
        }
    }

    // 存现场：下次启动要知道"上次应用的是哪条链"。
    // 存在 apply 里而不是前端，是因为预设应用 / 启动恢复 / 手动应用都会走这里，
    // 只在前端存容易漏（比如从中间栏点预设）。
    if (auto* dbContext = dllManager::getInstance().getDbContext()) {
        dbContext->pluginChainPresetManage.saveCurrentChain(filtered.dump());
    }

    Yvar droppedArray = Yvar::array();
    for (const auto& id : dropped) droppedArray.push_back(id);

    result.set("ok", true);
    result.set("error", "");
    result.set("dropped", droppedArray);
    return result;
}

void PluginChainController::setBypassAll(bool bypassAll) {
    if (!mSend) return;

    {
        std::lock_guard<std::mutex> lock{mMutex};
        if (mConfig.isObject()) mConfig.set("bypassAll", bypassAll);
    }

    Yvar envelope = Yvar::object();
    envelope.set(AudioMacro::pluginChainBypassAll, bypassAll);
    mSend(envelope.dump());
}

void PluginChainController::setSlotBypass(int slot, bool bypass) {
    if (!mSend) return;
    if (slot < 0 || slot >= kSlotCount) return;

    Yvar inner = Yvar::object();
    inner.set(AudioMacro::pluginChainSlot, slot);
    inner.set(AudioMacro::pluginChainBypass, bypass);

    Yvar envelope = Yvar::object();
    envelope.set(AudioMacro::pluginChainSlotBypass, inner);
    mSend(envelope.dump());
}

void PluginChainController::openEditor(int slot) {
    if (!mSend) return;
    if (slot < 0 || slot >= kSlotCount) return;

    Yvar inner = Yvar::object();
    inner.set(AudioMacro::pluginChainSlot, slot);

    Yvar envelope = Yvar::object();
    envelope.set(AudioMacro::pluginChainOpenEditor, inner);
    mSend(envelope.dump());
}

void PluginChainController::onStateFromAudio(const Yvar& state) {
    {
        std::lock_guard<std::mutex> lock{mMutex};
        mState = state;
        // 音频进程回话了 → 说明它扛过了这次下发，待观察名单可以清了
        mPendingIdentifiers.clear();
    }
}

Yvar PluginChainController::getState() {
    Yvar out = Yvar::object();
    std::lock_guard<std::mutex> lock{mMutex};
    if (mState.isObject()) {
        out.set("chain", mState);
    } else {
        out.set("chain", Yvar{});
    }
    return out;
}

Yvar PluginChainController::getConfig() {
    std::lock_guard<std::mutex> lock{mMutex};
    return mConfig;
}

void PluginChainController::banPendingAsRuntimeCrash() {
    std::set<std::string> pending;
    {
        std::lock_guard<std::mutex> lock{mMutex};
        pending.swap(mPendingIdentifiers);
    }
    if (pending.empty()) return;

    auto* dbContext = dllManager::getInstance().getDbContext();
    if (dbContext == nullptr) return;

    for (const auto& identifier : pending) {
        // filePath 这里拿不到（配置里可能有），留空不影响按 identifier 判定
        dbContext->pluginPolicyManage.ban(identifier, "", "runtime-crash");
        if (auto logger = spdlog::get(LogDllID))
            logger->warn("[PluginChain] 音频进程在下发后崩溃，已记录运行期崩溃: {}", identifier);
    }
}

void PluginChainController::resetAfterRestart() {
    // 先看"崩之前是不是正在加载插件"：是的话这是运行期崩溃，要记账
    banPendingAsRuntimeCrash();

    {
        std::lock_guard<std::mutex> lock{mMutex};
        mState = Yvar{};
    }

    // 刻意不重发 mConfig：音频进程刚因为某个插件死过一次，自动恢复它
    // 就是「崩溃→重启→再崩」（host-integration.md §6.2）。
    if (auto logger = spdlog::get(LogDllID))
        logger->warn("[PluginChain] 音频进程重启，链已清空，需要用户重新应用");
}
