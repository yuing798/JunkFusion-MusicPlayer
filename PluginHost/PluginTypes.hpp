/**
 * @brief 插件域的基础数据类型与枚举（**不含任何 JUCE 类型**）
 *
 * ── 设计目的 ────────────────────────────────────────────────────────────
 * 这一层是"插件扫描"唯一需要三方共享的部分：
 *
 *    PluginScanCommandLine  ──┐
 *    JunkFusionPluginScan.exe ─┼──> 本文件
 *    JunkFusionCoordinator   ──┤
 *    JunkFusionAudioProcess  ──┘（将来：加载插件时用）
 *
 * 为什么坚持用 std::string / POD 而不是 juce::String：
 *   1. 分类规则和扫描线路协议是**纯逻辑**，把它锁死在标准库上，才能在
 *      Test/test_plugin_host.cpp 里用裸 cl 直接编译测试，不用链接 JUCE；
 *   2. 扫描子进程要被任意第三方 DLL 污染，它加载的依赖越少越不容易被带崩。
 *   juce::PluginDescription 与 Meta 的转换只发生在 PluginEnumerate.cpp 一处。
 *
 * ── 术语 ────────────────────────────────────────────────────────────────
 *   "模块文件"(module) = 磁盘上的一个 .vst3（Windows 上是文件或 bundle 目录），
 *                        它可能导出 1 个或多个"插件类型"。
 *   "记录"(Record)     = 一个插件类型，扫描的最小单位，对应数据库 plugins 表一行。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     PluginHost::Record record;
 *     record.meta = {.name = "Surge XT", .isInstrument = true, ...};
 *     auto decision = PluginHost::classifyMeta(record.meta);   // PluginCategory.hpp
 *     record.type = decision.type;
 *     record.rule = decision.rule;
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace PluginHost {

    /** @brief 入桶的三大类，对应数据库 plugins.type */
    enum class Category {
        instrument, // 音源
        effect,     // 效果器
        other       // 兜底桶：元数据不可信的插件落这里，前端允许手动改
    };

    /**
     * @brief 命中分类规则时的证据，只用于日志
     *
     * 规格 §2 建议把命中的规则写进日志：出问题时能立刻判断是"元数据缺失"
     * 还是"通道数异常"，而不是盯着 other 干瞪眼。
     */
    enum class CategoryRule {
        isInstrument, // 元数据直接声明是音源
        categoryHint, // category 里含 Synth / Instrument
        noInput,      // 0 进 >0 出
        categoryFx,   // category 里含 Fx / Effect
        hasInput,     // 只要有输入就算效果器（监听/可视化 n进0出 也在链上）
        none          // 0 进 0 出且无任何线索
    };

    /** @brief 扫描结果状态，对应数据库 plugins.scanStatus */
    enum class ScanStatus {
        ok,
        failed
    };

    /**
     * @brief 一个插件类型的元数据（与 juce::PluginDescription 一一对应）
     *
     * @param category 元数据里的原始分类字符串（"Fx" / "Instrument|Synth" …），
     *                 和下面的 type 不是一回事：category 是插件自己报的，
     *                 type 是我们按 PluginCategory.cpp 的规则算出来的桶。
     */
    struct Meta {
        std::string name;
        std::string vendor;
        std::string category;
        std::string identifier; // PluginDescription::createIdentifierString()
        int numInputs{0};
        int numOutputs{0};
        bool isInstrument{false};
    };

    /** @brief 扫描产出的一条记录 = 数据库 plugins 表的一行 */
    struct Record {
        Meta meta;
        Category type{Category::other};
        CategoryRule rule{CategoryRule::none};
        int ordinal{0}; // 同一个模块文件里的第几个类型（VST3 通常只有 0）
        std::string error;    // type==failed 时的原因
        bool failed{false};
    };

    // ── 枚举与字符串互转（数据库/线路协议/前端都用同一套字面量）────────────

    inline std::string categoryToString(Category category) {
        switch (category) {
            case Category::instrument:
                return "instrument";
            case Category::effect:
                return "effect";
            case Category::other:
                return "other";
        }
        return "other";
    }

    /** @brief 前端的"手动改 type"入口，非法值一律拒绝（不改库） */
    inline bool categoryFromString(std::string_view text, Category& out) {
        if (text == "instrument") {
            out = Category::instrument;
            return true;
        }
        if (text == "effect") {
            out = Category::effect;
            return true;
        }
        if (text == "other") {
            out = Category::other;
            return true;
        }
        return false;
    }

    inline std::string ruleToString(CategoryRule rule) {
        switch (rule) {
            case CategoryRule::isInstrument:
                return "isInstrument";
            case CategoryRule::categoryHint:
                return "category";
            case CategoryRule::noInput:
                return "no-input";
            case CategoryRule::categoryFx:
                return "category-fx";
            case CategoryRule::hasInput:
                return "has-input";
            case CategoryRule::none:
                return "none";
        }
        return "none";
    }

    inline std::string scanStatusToString(ScanStatus status) {
        return status == ScanStatus::ok ? "ok" : "failed";
    }

} // namespace PluginHost
