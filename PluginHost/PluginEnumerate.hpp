/**
 * @brief 用 JUCE 的**无头**格式管理器把一个 `.vst3` 模块枚举成插件元数据
 *
 * ── 它做什么 ────────────────────────────────────────────────────────────
 *   输入：一个模块路径（bundle 目录或裸 DLL）
 *   输出：这个模块导出的所有插件类型（VST3 实测 1:1，但 VST3 shell 会多个）
 *
 * 规格 §1.2 说得很明确：**枚举是实例化的前提**，不存在"跳过元数据只做实例化"
 * 这条路——`createPluginInstance()` 的入参 `PluginDescription` 只能由
 * `findAllTypesForFile()` 得到。所以默认扫描就只有这一步，一遍过。
 *
 * ── 为什么是 headless ──────────────────────────────────────────────────
 *   juce_audio_processors_headless 只依赖 juce_audio_basics + juce_events，
 *   不带 GUI。扫描子进程并不需要编辑器，用 headless 版本可以让子进程：
 *     · 不链接任何窗口/图形代码（崩溃面小、加载快）；
 *     · 不会被插件的 modal 对话框卡住（`addHeadlessDefaultFormatsToManager`
 *       装出来的实例 `hasEditor()` 永远返回 false）。
 *   音频进程里将来要开编辑器时，改用带 UI 的那个版本即可，本类的接口不变。
 *
 * ── 注意：这里不做分类 ──────────────────────────────────────────────────
 *   分类是"策略"（PluginCategory.cpp），本文件只负责"反射"。
 *   调用方拿到 `Record` 后自己调 `classifyMeta` 填 type/rule。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     PluginHost::Enumerator enumerator;
 *     std::vector<PluginHost::Record> records;
 *     std::string error;
 *     if (!enumerator.enumerate(moduleFile, records, error)) { ...失败... }
 *     for (auto& r : records) { auto d = classifyMeta(r.meta); r.type = d.type; }
 */
#pragma once

#include "./PluginTypes.hpp"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <string>
#include <vector>

namespace PluginHost {

    class Enumerator {
    public:
        Enumerator();

        // AudioPluginFormatManager 本身不可拷贝，这里只是把意图写明
        Enumerator(const Enumerator&) = delete;
        Enumerator& operator=(const Enumerator&) = delete;

        /**
         * @brief 枚举一个模块文件
         *
         * @param module  模块路径（`.vst3` 文件或 bundle 目录）
         * @param out     产出的记录（meta 已填好，type/rule 仍是默认值）
         * @param error   失败原因（UTF-8）
         * @return 至少拿到一个插件类型返回 true
         */
        bool enumerate(const juce::File& module, std::vector<Record>& out, std::string& error);

        /**
         * @brief 规格 §3.2 的可选路径：实例化一次读**实测**通道数
         *
         * 只用于排查（默认扫描不走这条路，见规格 §1.5：实例化让单插件成本
         * 变成 2.6 倍，而分类结果 0 处变化）。
         *
         * 刻意不 `prepareToPlay`：读总线布局不需要启动 DSP，省一次重分配。
         */
        bool measureChannels(
            const juce::File& module,
            const std::string& identifier,
            int& numInputs,
            int& numOutputs,
            std::string& error
        );

    private:
        /** @brief 找出所有认领这个文件的格式，收集它们的 PluginDescription */
        bool collectDescriptions(
            const juce::File& module,
            juce::OwnedArray<juce::PluginDescription>& out,
            std::string& error
        ) const;

        juce::AudioPluginFormatManager mFormats;
    };

} // namespace PluginHost
