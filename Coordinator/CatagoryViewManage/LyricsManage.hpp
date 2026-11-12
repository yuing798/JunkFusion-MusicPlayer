/**
 * @brief 歌词的数据库读写（lyrics 表）
 *
 * ── 表结构 ────────────────────────────────────────────────────────────
 *   lyrics(lyricId, songId, kind, format, content, sourcePath, mtime)
 *   一首歌最多两条：kind = 'original' / 'translation'（UNIQUE(songId,kind)）
 *
 * 存的是**原始文本**，不是解析结果。解析在读取时做（几 KB 的文本 <1ms），
 * 这样歌词格式升级不用迁移数据，用户在外部改文件后重新导入也只是换 text。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     lyricsManage.upsert(songId, "original", "lrc", text, path, mtime);
 *     auto doc = lyricsManage.get(songId);   // 已合并翻译
 */
#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Model/LyricInfo.hpp"
#include <cstdint>
#include <optional>
#include <string>

class LyricsManage {
public:
    /// kind 取值
    static constexpr const char* kKindOriginal = "original";
    static constexpr const char* kKindTranslation = "translation";

    explicit LyricsManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    /**
     * @brief 写入/覆盖某首歌的歌词
     *
     * @param format "lrc" / "elrc"（只作记录，解析器自己会判断）
     * @param mtime 源文件修改时间（毫秒），以后做"文件变了就重读"用
     */
    void upsert(
        int64_t songId,
        const std::string& kind,
        const std::string& format,
        const std::string& content,
        const std::string& sourcePath,
        int64_t mtime
    );

    /// 读歌词（原文 + 翻译合并解析好），没有歌词返回 nullopt
    std::optional<LyricDocument> get(int64_t songId);

    /// 这首歌有没有原文歌词
    bool hasLyrics(int64_t songId);

    /// 删掉某首歌的全部歌词（歌被删时顺带清理；外键 CASCADE 也会处理）
    void remove(int64_t songId);

private:
    /// 读某一类歌词的原始文本
    std::string readContent(int64_t songId, const std::string& kind);

    CenterDbManage& dbManage;
};
