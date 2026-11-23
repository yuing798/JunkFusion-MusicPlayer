/**
 * @brief 全局效果器链（音频进程侧）：8 个插槽 + 整链旁通 + 单槽旁通
 *
 * ── 它插在哪 ──────────────────────────────────────────────────────────
 *   deck0/deck1 → CrossFadeArea → **本类** → Eq → 主音量 → ...
 *   交叉淡化之后的单条路径，所以：
 *     · 只需要 1 套实例；
 *     · **不需要 PDC**（没有并行路径）；
 *     · 块大小固定 = 设备块（441），插件不会看到变长的块。
 *   详细理由见 docs/audio-routing.md §4.1。
 *
 * ── 线程模型（重要）───────────────────────────────────────────────────
 *   `process()`      音频线程：只遍历插槽、调 processBlock，不分配、不加锁
 *   `applyConfig()`  puller 线程：实例化插件（会 LoadLibrary，几十~几百毫秒）
 *   `setBypassAll()/setSlotBypass()` 任意线程：只写 atomic
 *
 *   整链**换配置**不是原地改，而是构建一条新链再整体换指针（GodProcessor 里做）。
 *   因为插件实例不能在线程 A 用、线程 B 删。
 *
 * ── 通道约定（v1）─────────────────────────────────────────────────────
 *   只接受 2 进 2 出的插件。通道数不匹配的插件在加载阶段就被拒绝并给出原因，
 *   而不是让它输出静音（静音最难排查）。preflight 报告里已经有通道数，
 *   前端在插入前就能提示。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     PluginChain chain;
 *     chain.prepare(44100, 441, 2);
 *     chain.applyConfig(configYvar, factory, 44100, 441, error);
 *     // 音频线程：
 *     chain.process(buffer);
 */
#pragma once

#include "PluginHost/PluginInstanceFactory.hpp"
#include "Utils/Yvar.hpp"

#include "juce_audio_processors/juce_audio_processors.h"

#include <array>
#include <atomic>
#include <memory>
#include <string>

namespace PluginHost {

    /** @brief 一个插槽 = 一个插件实例 + 旁通开关 */
    class PluginSlot {
    public:
        /** @brief 加载插件；失败返回 false 并把原因写进 error（不抛异常） */
        bool load(
            PluginInstanceFactory& factory,
            const std::string& filePath,
            const std::string& identifier,
            double sampleRate,
            int blockSize,
            int channels,
            std::string& error
        );

        void release();

        bool isLoaded() const noexcept { return mInstance != nullptr; }
        void setBypass(bool bypass) noexcept { mBypass.store(bypass, std::memory_order_relaxed); }
        bool isBypassed() const noexcept { return mBypass.load(std::memory_order_relaxed); }

        /** @brief 音频线程：处理一块（原地） */
        void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

        int latencySamples() const noexcept { return mLatency; }

        juce::String name() const;
        juce::String identifier() const;
        juce::String filePath() const;
        juce::String error() const;
        void setError(const juce::String& error) { mError = error; }

        juce::String getStateBase64() const;
        bool setStateBase64(const juce::String& base64);

    private:
        std::unique_ptr<juce::AudioPluginInstance> mInstance;
        std::atomic<bool> mBypass{false};
        int mLatency{0};
        juce::String mName;
        juce::String mIdentifier;
        juce::String mFilePath;
        juce::String mError;
    };

    /** @brief 8 个插槽的全局链 */
    class PluginChain {
    public:
        static constexpr int kSlotCount = 8;

        /** @brief 音频线程：依次过每个未旁通、已加载的插槽 */
        void process(juce::AudioBuffer<float>& buffer);

        void setBypassAll(bool bypass) noexcept {
            mBypassAll.store(bypass, std::memory_order_relaxed);
        }
        bool isBypassAll() const noexcept { return mBypassAll.load(std::memory_order_relaxed); }

        void setSlotBypass(int slot, bool bypass) noexcept;
        bool isAnyLoaded() const;

        int totalLatencySamples() const;

        /**
         * @brief 按配置重建整条链（**只能在非音频线程调**）
         *
         * 配置形状：
         * {"bypassAll":false,
         *  "slots":[{"index":0,"filePath":"...","identifier":"...","bypass":false,"state":"base64"},
         *           ... 最多 8 条]}
         * 缺失的 index = 空槽。每个槽单独成败，失败原因进 status()。
         *
         * @return 至少有一个槽加载成功返回 true
         */
        bool applyConfig(
            const Yvar& config,
            PluginInstanceFactory& factory,
            double sampleRate,
            int blockSize,
            int channels
        );

        /** @brief 给 Coordinator / 前端看的当前状态 */
        Yvar statusJson() const;

        /** @brief 每块都要清空的 MIDI 缓冲（乐器插件需要；效果器插件忽略） */
        juce::MidiBuffer mMidi;

    private:
        std::array<std::unique_ptr<PluginSlot>, kSlotCount> mSlots;
        std::atomic<bool> mBypassAll{false};
    };

} // namespace PluginHost
