/**
 * @brief 歌词的解析与数据模型（不碰数据库、不碰文件系统）
 *
 * ── 支持哪些格式 ──────────────────────────────────────────────────────
 *   · 标准 LRC：`[mm:ss.xx]歌词`
 *   · 增强 LRC（A2）：行内带 `<mm:ss.xx>` 逐字时间
 *   · 一行多时间戳：`[00:12.00][00:45.00]副歌` → 拆成两条
 *   · 元数据标签（ti/ar/al/by/offset）解析后忽略，不会当成时间
 *
 * ── 翻译怎么合 ────────────────────────────────────────────────────────
 * 原文和翻译是两份独立的 LRC，按**起始时间**配对（±250ms 容差），
 * 配上的行会填进 translation 字段，前端一行原文一行翻译地渲染。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     auto doc = LyricParser::parse(originalText, translationText);
 *     if (doc) envelope.set("lyric", doc->toJson());
 *
 * 解析是纯 CPU 操作（几 KB 文本 <1ms），所以不缓存解析结果，
 * 每次从库里读出原文再解析，改格式也不用迁移数据。
 */
#pragma once

#include "Utils/Yvar.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// 逐字时间（只对增强 LRC 有值）
struct LyricWord {
    int32_t startMs{0};
    int32_t endMs{0};
    std::string text;
};

/// 一行歌词
struct LyricLine {
    int32_t startMs{0};
    std::string text;        // 原文
    std::string translation; // 翻译（可能为空）
    std::vector<LyricWord> words;
};

class LyricDocument {
public:
    std::vector<LyricLine> lines;
    bool hasTranslation{false};

    bool isEmpty() const noexcept { return lines.empty(); }

    /** @brief 转成给前端的 JSON：{ "hasTranslation":bool, "lines":[{startMs,text,translation,words}] } */
    Yvar toJson() const;
};

class LyricParser {
public:
    /**
     * @brief 解析原文 + 可选的翻译文本
     *
     * @param original 原文 LRC 内容
     * @param translation 翻译 LRC 内容（可以传空）
     * @return 一条有效歌词都没有时返回 nullopt
     */
    static std::optional<LyricDocument>
        parse(const std::string& original, const std::string& translation);

private:
    /// 一行歌词（解析中间结果）
    struct ParsedLine {
        int32_t startMs{0};
        std::string text;
        std::vector<LyricWord> words;
    };

    /// 按行解析，返回按时间排好序的结果
    static std::vector<ParsedLine> parseLines(const std::string& text);

    /// 解析 `[mm:ss.xx]` / `[mm:ss:xx]` / `[mm:ss]`；不是时间标签就返回 false
    static bool parseTimeTag(const std::string& tag, int32_t& outMs);

    /// 处理增强 LRC 的行内 `<mm:ss.xx>`，同时返回去掉标记的纯文本
    static std::string parseWords(const std::string& body, std::vector<LyricWord>& outWords);
};
