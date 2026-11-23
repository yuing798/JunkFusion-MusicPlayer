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

Yvar PluginChainController::filterByPolicy(
    const Yvar& config,
    std::vector<std::string>& dropped
) {
    Yvar out = Yvar::object();
    out.set("bypassAll", config.read("bypassAll").toBool());

    auto* dbContext = dllManager::getInstance().getDbContext();
    PluginPolicyManage* policy = dbContext != nullptr ? &dbContext->pluginPolicyManage : nullptr;

    // 黑名单只取一次，别在循环里反复查库
    std::vector<std::string> banned;
    if (policy != nullptr) banned = policy->getBlacklisted();

    Yvar slots = Yvar::array();
    const auto input = config.read("slots");
    for (int i = 0; i < input.size(); ++i) {
        const auto item = input.read(i);
        const std::string identifier = item.read("identifier").toString();

        if (policy != nullptr && !identifier.empty()) {
            const bool isBanned =
                std::find(banned.begin(), banned.end(), identifier) != banned.end();
            // 白名单优先级最高：它只解除"自动禁止"，不改变别的规则
            if (isBanned && !policy->isWhitelisted(identifier)) {
                dropped.push_back(identifier);
                if (auto logger = spdlog::get(LogDllID))
                    logger->warn("[PluginChain] 跳过黑名单插件: {}", identifier);
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

void PluginChainController::onStateFromAudio(const Yvar& state) {
    std::lock_guard<std::mutex> lock{mMutex};
    mState = state;
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

void PluginChainController::resetAfterRestart() {
    {
        std::lock_guard<std::mutex> lock{mMutex};
        mState = Yvar{};
    }

    // 刻意不重发 mConfig：音频进程刚因为某个插件死过一次，自动恢复它
    // 就是「崩溃→重启→再崩」（host-integration.md §6.2）。
    if (auto logger = spdlog::get(LogDllID))
        logger->warn("[PluginChain] 音频进程重启，链已清空，需要用户重新应用");
}
