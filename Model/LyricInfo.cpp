#include "./LyricInfo.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace {

/// JSON 键（歌词只在这一处出现，先不单独开一个 codegen 宏文件）
constexpr const char* kHasTranslation = "hasTranslation";
constexpr const char* kLines = "lines";
constexpr const char* kStartMs = "startMs";
constexpr const char* kEndMs = "endMs";
constexpr const char* kText = "text";
constexpr const char* kTranslation = "translation";
constexpr const char* kWords = "words";

/// 翻译行和原文行的时间容差（LRC 手写文件经常差几十毫秒）
constexpr int32_t kTranslationToleranceMs = 250;

bool isAllDigits(const std::string& text) {
    if (text.empty()) return false;
    for (const char ch : text) {
        if (std::isdigit(static_cast<unsigned char>(ch)) == 0) return false;
    }
    return true;
}

int toInt(const std::string& text) {
    return static_cast<int>(std::strtol(text.c_str(), nullptr, 10));
}

} // namespace

bool LyricParser::parseTimeTag(const std::string& tag, int32_t& outMs) {
    // 形如 mm:ss.xx / mm:ss:xx / mm:ss，其它（ti/ar/al/by/offset）一律拒绝
    const size_t firstColon = tag.find(':');
    if (firstColon == std::string::npos) return false;

    const std::string minutesPart = tag.substr(0, firstColon);
    const std::string rest = tag.substr(firstColon + 1);

    const size_t secondSep = rest.find_first_of(".:");
    const std::string secondsPart =
        (secondSep == std::string::npos) ? rest : rest.substr(0, secondSep);
    const std::string fractionPart =
        (secondSep == std::string::npos) ? std::string{} : rest.substr(secondSep + 1);

    if (!isAllDigits(minutesPart) || !isAllDigits(secondsPart)) return false;
    if (!fractionPart.empty() && !isAllDigits(fractionPart)) return false;

    const int minutes = toInt(minutesPart);
    const int seconds = toInt(secondsPart);
    if (seconds >= 60) return false; // [al:xxx] 之类的误判兜底

    int millis = 0;
    if (!fractionPart.empty()) {
        if (fractionPart.size() == 1) {
            millis = toInt(fractionPart) * 100;
        } else if (fractionPart.size() == 2) {
            millis = toInt(fractionPart) * 10;
        } else {
            millis = toInt(fractionPart.substr(0, 3));
        }
    }

    outMs = minutes * 60000 + seconds * 1000 + millis;
    return true;
}

std::string LyricParser::parseWords(const std::string& body, std::vector<LyricWord>& outWords) {
    // body 形如：`她说<00:12.34>你好<00:13.00>世界`（标记前的文本属于第一段）
    std::string plainText;
    struct Segment {
        int32_t startMs;
        std::string text;
    };
    std::vector<Segment> segments;

    size_t cursor = 0;
    std::string pendingText;
    int32_t pendingStart = 0;

    while (cursor < body.size()) {
        const size_t tagBegin = body.find('<', cursor);
        if (tagBegin == std::string::npos) {
            pendingText += body.substr(cursor);
            break;
        }

        const size_t tagEnd = body.find('>', tagBegin);
        if (tagEnd == std::string::npos) {
            pendingText += body.substr(cursor);
            break;
        }

        pendingText += body.substr(cursor, tagBegin - cursor);

        int32_t wordStart = 0;
        const std::string tag = body.substr(tagBegin + 1, tagEnd - tagBegin - 1);
        if (parseTimeTag(tag, wordStart)) {
            if (!pendingText.empty()) {
                segments.push_back({pendingStart, pendingText});
            }
            pendingText.clear();
            pendingStart = wordStart;
        } else {
            // 不是时间标签就原样保留（比如 HTML 风格的 <b>）
            pendingText += body.substr(tagBegin, tagEnd - tagBegin + 1);
        }

        cursor = tagEnd + 1;
    }

    if (!pendingText.empty()) {
        segments.push_back({pendingStart, pendingText});
    }

    for (const auto& segment : segments) plainText += segment.text;

    // 每段的结束时间 = 下一段的开始时间；最后一段给 5 秒兜底
    for (size_t i = 0; i < segments.size(); ++i) {
        LyricWord word;
        word.startMs = segments[i].startMs;
        word.endMs = (i + 1 < segments.size()) ? segments[i + 1].startMs
                                               : segments[i].startMs + 5000;
        word.text = segments[i].text;
        outWords.push_back(std::move(word));
    }

    // 只有一段且没有时间标记时，words 没意义，丢掉
    if (outWords.size() <= 1) outWords.clear();

    return plainText;
}

std::vector<LyricParser::ParsedLine> LyricParser::parseLines(const std::string& text) {
    std::vector<ParsedLine> result;

    size_t cursor = 0;
    while (cursor < text.size()) {
        size_t lineEnd = text.find('\n', cursor);
        if (lineEnd == std::string::npos) lineEnd = text.size();

        std::string line = text.substr(cursor, lineEnd - cursor);
        cursor = lineEnd + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // 行首可能有多个标签：[00:12.00][00:45.00]歌词
        std::vector<int32_t> timestamps;
        size_t index = 0;
        while (index < line.size() && line[index] == '[') {
            const size_t close = line.find(']', index);
            if (close == std::string::npos) break;

            int32_t startMs = 0;
            if (parseTimeTag(line.substr(index + 1, close - index - 1), startMs)) {
                timestamps.push_back(startMs);
            }
            index = close + 1;
        }

        if (timestamps.empty()) continue; // 纯元数据行 / 没有时间轴的行

        const std::string body = line.substr(index);

        std::vector<LyricWord> words;
        const std::string plainText = parseWords(body, words);

        std::string trimmed = plainText;
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
            trimmed.erase(trimmed.begin());
        }
        while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
            trimmed.pop_back();
        }

        if (trimmed.empty()) continue; // 纯时间戳的空行（间奏）先跳过

        for (const int32_t startMs : timestamps) {
            result.push_back({startMs, trimmed, words});
        }
    }

    std::sort(result.begin(), result.end(), [](const ParsedLine& a, const ParsedLine& b) {
        return a.startMs < b.startMs;
    });

    return result;
}

std::optional<LyricDocument> LyricParser::parse(
    const std::string& original,
    const std::string& translation
) {
    const auto originalLines = parseLines(original);
    if (originalLines.empty()) return std::nullopt;

    // 翻译：起始时间 → 文本（同一时间只留第一条，重复的翻译行没有意义）
    std::unordered_map<int32_t, std::string> translationByTime;
    if (!translation.empty()) {
        for (const auto& line : parseLines(translation)) {
            translationByTime.emplace(line.startMs, line.text);
        }
    }

    LyricDocument document;
    document.lines.reserve(originalLines.size());

    for (const auto& parsed : originalLines) {
        LyricLine line;
        line.startMs = parsed.startMs;
        line.text = parsed.text;
        line.words = parsed.words;

        if (!translationByTime.empty()) {
            // 精确命中优先，否则在容差内找最近的一条
            auto exact = translationByTime.find(parsed.startMs);
            if (exact != translationByTime.end()) {
                line.translation = exact->second;
            } else {
                int32_t bestDelta = kTranslationToleranceMs + 1;
                for (const auto& [timeMs, text] : translationByTime) {
                    const int32_t delta = std::abs(timeMs - parsed.startMs);
                    if (delta < bestDelta) {
                        bestDelta = delta;
                        line.translation = text;
                    }
                }
                if (bestDelta > kTranslationToleranceMs) line.translation.clear();
            }
        }

        if (!line.translation.empty()) document.hasTranslation = true;
        document.lines.push_back(std::move(line));
    }

    return document;
}

Yvar LyricDocument::toJson() const {
    Yvar object = Yvar::object();
    object.set(kHasTranslation, hasTranslation);

    Yvar lineArray = Yvar::array();
    for (const auto& line : lines) {
        Yvar item = Yvar::object();
        item.set(kStartMs, line.startMs);
        item.set(kText, line.text);
        if (!line.translation.empty()) {
            item.set(kTranslation, line.translation);
        }

        if (!line.words.empty()) {
            Yvar wordArray = Yvar::array();
            for (const auto& word : line.words) {
                Yvar wordItem = Yvar::object();
                wordItem.set(kStartMs, word.startMs);
                wordItem.set(kEndMs, word.endMs);
                wordItem.set(kText, word.text);
                wordArray.push(wordItem);
            }
            item.set(kWords, wordArray);
        }

        lineArray.push(item);
    }
    object.set(kLines, lineArray);

    return object;
}
