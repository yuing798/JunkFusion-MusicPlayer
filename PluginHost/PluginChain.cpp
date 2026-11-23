#include "PluginHost/PluginChain.hpp"

#include "Utils/constants.h"

#include <spdlog/spdlog.h>

namespace PluginHost {

    // ════════════════════════════════════════════════════════════
    // PluginSlot
    // ════════════════════════════════════════════════════════════
    bool PluginSlot::load(
        PluginInstanceFactory& factory,
        const std::string& filePath,
        const std::string& identifier,
        double sampleRate,
        int blockSize,
        int channels,
        std::string& error
    ) {
        release();
        mFilePath = juce::String::fromUTF8(filePath.c_str());
        mIdentifier = juce::String::fromUTF8(identifier.c_str());

        auto instance = factory.create(filePath, identifier, sampleRate, blockSize, error);
        if (instance == nullptr) {
            mError = juce::String::fromUTF8(error.c_str());
            return false;
        }

        // ── 只保留主总线，把侧链/辅助输出全关掉 ──
        // 侧链插件（TDR Nova 实测 3 进 2 出）不关侧链的话总通道数对不上，
        // 直接判定"不匹配"会把最常用的一批 EQ/压缩器挡在门外。
        // 关掉之后它们就是标准 2/2。
        {
            auto layout = instance->getBusesLayout();
            const auto target = juce::AudioChannelSet::canonicalChannelSet(channels);
            const auto disabled = juce::AudioChannelSet::disabled();

            for (int i = 0; i < layout.inputBuses.size(); ++i) {
                layout.inputBuses.set(i, i == 0 ? target : disabled);
            }
            for (int i = 0; i < layout.outputBuses.size(); ++i) {
                layout.outputBuses.set(i, i == 0 ? target : disabled);
            }
            instance->setBusesLayout(layout);
        }

        // v1 只支持和总线一致的通道数：不匹配就明确拒绝，
        // 不要让它"能加载但没声"——那种最难排查
        const int ins = instance->getTotalNumInputChannels();
        const int outs = instance->getTotalNumOutputChannels();
        if (ins != channels || outs != channels) {
            error = "通道数不匹配：插件是 " + std::to_string(ins) + "/" + std::to_string(outs) +
                    "，效果器链只接受 " + std::to_string(channels) + "/" +
                    std::to_string(channels);
            mError = juce::String::fromUTF8(error.c_str());
            return false;
        }

        mName = instance->getName();
        instance->setPlayConfigDetails(channels, channels, sampleRate, blockSize);
        instance->prepareToPlay(sampleRate, blockSize);

        // 延迟在 prepare 之后才有值（TDR Nova：prepare 前 0、后 184）
        mLatency = instance->getLatencySamples();

        mInstance = std::move(instance);
        mError.clear();
        return true;
    }

    void PluginSlot::release() {
        if (mInstance != nullptr) {
            mInstance->releaseResources();
            mInstance.reset();
        }
        mLatency = 0;
    }

    void PluginSlot::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
        if (mInstance == nullptr) return;
        if (mBypass.load(std::memory_order_relaxed)) return;

        mInstance->processBlock(buffer, midi);

        // 运行中延迟会变（CHOWTapeModel 39→43），这里顺手续一下；
        // 全局链是单路径，不需要 PDC，只是让 UI 显示的数字准确
        const int current = mInstance->getLatencySamples();
        if (current != mLatency) mLatency = current;
    }

    juce::String PluginSlot::name() const {
        return mInstance != nullptr ? mName : juce::String{};
    }

    juce::String PluginSlot::identifier() const { return mIdentifier; }

    juce::String PluginSlot::filePath() const { return mFilePath; }

    juce::String PluginSlot::error() const { return mError; }

    juce::String PluginSlot::getStateBase64() const {
        if (mInstance == nullptr) return {};
        juce::MemoryBlock block;
        mInstance->getStateInformation(block);
        return block.toBase64Encoding();
    }

    bool PluginSlot::setStateBase64(const juce::String& base64) {
        if (mInstance == nullptr || base64.isEmpty()) return false;

        juce::MemoryBlock block;
        if (!block.fromBase64Encoding(base64)) return false;
        mInstance->setStateInformation(block.getData(), static_cast<int>(block.getSize()));
        return true;
    }

    // ════════════════════════════════════════════════════════════
    // PluginChain
    // ════════════════════════════════════════════════════════════
    void PluginChain::process(juce::AudioBuffer<float>& buffer) {
        if (mBypassAll.load(std::memory_order_relaxed)) return;

        mMidi.clear();
        for (auto& slot : mSlots) {
            if (slot == nullptr) continue;
            if (!slot->isLoaded()) continue;
            slot->process(buffer, mMidi);
        }
    }

    void PluginChain::setSlotBypass(int slot, bool bypass) noexcept {
        if (slot < 0 || slot >= kSlotCount) return;
        if (mSlots[static_cast<size_t>(slot)] == nullptr) return;
        mSlots[static_cast<size_t>(slot)]->setBypass(bypass);
    }

    bool PluginChain::isAnyLoaded() const {
        for (const auto& slot : mSlots) {
            if (slot != nullptr && slot->isLoaded()) return true;
        }
        return false;
    }

    int PluginChain::totalLatencySamples() const {
        int total = 0;
        for (const auto& slot : mSlots) {
            if (slot == nullptr || !slot->isLoaded() || slot->isBypassed()) continue;
            total += slot->latencySamples();
        }
        return total;
    }

    bool PluginChain::applyConfig(
        const Yvar& config,
        PluginInstanceFactory& factory,
        double sampleRate,
        int blockSize,
        int channels
    ) {
        // 清空旧槽（这个对象本身就是"离线构建的新链"，不会和音频线程共享，
        // 见 GodProcessor 里换链的那套流程）
        for (auto& slot : mSlots) {
            if (slot != nullptr) slot->release();
            slot.reset();
        }
        setBypassAll(config.read("bypassAll").toBool());

        const auto slots = config.read("slots");
        const int count = slots.size();
        bool anyLoaded = false;

        for (int i = 0; i < count; ++i) {
            const auto item = slots.read(i);
            const int index = item.read("index").toInt();
            if (index < 0 || index >= kSlotCount) continue;

            const std::string filePath = item.read("filePath").toString();
            const std::string identifier = item.read("identifier").toString();
            if (filePath.empty()) continue;

            auto slot = std::make_unique<PluginSlot>();
            slot->setBypass(item.read("bypass").toBool());

            std::string error;
            const bool ok = slot->load(
                factory,
                filePath,
                identifier,
                sampleRate,
                blockSize,
                channels,
                error
            );

            if (ok) {
                const juce::String state = juce::String::fromUTF8(
                    item.read("state").toString().c_str()
                );
                slot->setStateBase64(state);
                anyLoaded = true;
            }

            mSlots[static_cast<size_t>(index)] = std::move(slot);
        }

        return anyLoaded;
    }

    Yvar PluginChain::statusJson() const {
        Yvar out = Yvar::object();
        out.set("bypassAll", isBypassAll());
        out.set("anyLoaded", isAnyLoaded());
        out.set("latency", totalLatencySamples());

        Yvar arr = Yvar::array();
        for (int i = 0; i < kSlotCount; ++i) {
            Yvar item = Yvar::object();
            item.set("index", i);

            const auto& slot = mSlots[static_cast<size_t>(i)];
            const bool loaded = slot != nullptr && slot->isLoaded();
            item.set("loaded", loaded);
            if (slot != nullptr) {
                item.set("bypass", slot->isBypassed());
                item.set("name", slot->name().toStdString());
                item.set("identifier", slot->identifier().toStdString());
                item.set("filePath", slot->filePath().toStdString());
                item.set("latency", slot->latencySamples());
                item.set("error", slot->error().toStdString());
            }
            arr.push(item);
        }
        out.set("slots", arr);
        return out;
    }

} // namespace PluginHost
