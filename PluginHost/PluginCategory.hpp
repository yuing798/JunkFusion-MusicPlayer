/**
 * @brief 插件分类规则：把"元数据 + 通道数"判成一个桶（音源/效果器/其他）
 *
 * ── 为什么需要单独一个文件 ──────────────────────────────────────────────
 * 这是整条扫描链上**唯一会做主观判断**的地方，判错了整个插件库的分类就废了。
 * 把它从枚举、入库、进程监管里剥出来，一是可以在 Test/ 里穷举边界用例，
 * 二是将来调规则时只动这一个文件。
 *
 * ── 规则（定稿，严格按优先级，命中即停）──────────────────────────────────
 *   1. isInstrument == true                         → 音源
 *   2. category 含 "Synth" 或 "Instrument"（忽略大小写）→ 音源
 *   3. 输入 == 0 且 输出 > 0                        → 音源
 *   4. category 含 "Fx" 或 "Effect"（忽略大小写）     → 效果器
 *   5. 输入 > 0                                     → 效果器
 *   6. 其余（0 进 0 出）                             → 其他
 *
 * ── 为什么 1、2 必须在 3、4、5 之前（不能调换）──────────────────────────
 *   带音频输入的音源（Surge XT 报 2 进 2 出）和效果器的拓扑**完全一样**，
 *   光看通道数分不出来，只能靠元数据先拦住。这条顺序是实测（11 个真实插件）
 *   之后定下来的，改动前请先读 docs/main-project-scan-spec.md §2。
 *
 * ── 规则 4 为什么存在（参考实现里没有）──────────────────────────────────
 *   规格给的参考实现是 5 条（少了 category 含 Fx 这一条）。表格里明确列了它，
 *   且它只在"0 进 0 出但自称 Fx"这种元数据残缺的插件上生效——把这种塞进 other
 *   不如塞进 effect（other 是给人看的兜底桶）。挪到第 5 条之后也不会改变
 *   任何"有输入"插件的判定，所以不影响任何验收用例。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     auto d = PluginHost::classify(true, "Instrument|Synth", 2, 2);
 *     // → {Category::instrument, CategoryRule::isInstrument}
 *
 *     auto d2 = PluginHost::classifyMeta(meta);   // 元数据一起传的便捷版
 */
#pragma once

#include "./PluginTypes.hpp"

namespace PluginHost {

    struct CategoryDecision {
        Category type{Category::other};
        CategoryRule rule{CategoryRule::none};
    };

    /**
     * @brief 按 §2 的 6 条规则判定
     *
     * @param isInstrument juce::PluginDescription::isInstrument
     * @param category     juce::PluginDescription::category（可为空）
     * @param numInputs    默认路径用元数据通道数；实例化模式才换成实测值
     * @param numOutputs   同上
     */
    CategoryDecision classify(
        bool isInstrument,
        std::string_view category,
        int numInputs,
        int numOutputs
    );

    /** @brief 直接吃 Meta 的便捷版 */
    CategoryDecision classifyMeta(const Meta& meta);

    /** @brief 给日志用的一行摘要，如 "instrument(isInstrument) 2in/2out" */
    std::string describeDecision(
        const Meta& meta,
        const CategoryDecision& decision
    );

} // namespace PluginHost
