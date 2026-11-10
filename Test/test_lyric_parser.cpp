/**
 * @brief 歌词解析器测试（Model/LyricInfo.cpp）
 *
 * ── 测什么 ────────────────────────────────────────────────────────────
 *   LyricParser::parse() 对 LRC / 增强 LRC / 多时间戳 / 翻译合并 /
 *   脏数据（元数据标签、纯文本行）的处理是否正确。
 *
 * ── 通过标准 ──────────────────────────────────────────────────────────
 *   1. 标准 LRC：行数、时间戳（毫秒）、文本完全一致；
 *   2. 一行多时间戳：拆成多条，时间正确；
 *   3. 增强 LRC：行内逐字时间被解析成 words，纯文本去掉标记；
 *   4. 翻译：按起始时间配对（±250ms 容差），hasTranslation 正确；
 *   5. 元数据标签（ti/ar/al/by/offset）和无时间轴的行被忽略；
 *   6. 一条有效行都没有时返回 nullopt；
 *   7. 输出按时间升序；
 *   8. toJson() 的字段名/结构符合前端约定。
 *
 * ── 为什么要测 ────────────────────────────────────────────────────────
 *   歌词解析是纯字符串处理，出错不会崩，只会"看起来不对"（时间偏移、
 *   翻译错位、逐字高亮跳字），而且 LRC 的来源五花八门，手写文件经常有
 *   各种脏格式。这个模块没有别的验证手段，所以用固定样例锁住行为。
 *
 * ── 怎么跑 ────────────────────────────────────────────────────────────
 *   cl /utf-8 /std:c++20 /EHsc /I <项目根> /I <vcpkg include> ^
 *      Test\test_lyric_parser.cpp Model\LyricInfo.cpp Utils\Yvar.cpp ^
 *      /Fe:build\test_lyric_parser.exe
 *   （不依赖 JUCE / SQLite / FFmpeg，直接编译真实源码；项目是 C++20）
 */

#include "Model/LyricInfo.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace {

int gFailures = 0;
int gChecks = 0;

void check(bool condition, const char* what, int line) {
    ++gChecks;
    if (condition) return;
    ++gFailures;
    std::printf("  [FAIL] line %d: %s\n", line, what);
}

#define CHECK(expr) check((expr), #expr, __LINE__)

bool near(int actual, int expected, int tolerance = 0) {
    return std::abs(actual - expected) <= tolerance;
}

// ── 1. 标准 LRC ────────────────────────────────────────────────────────
void testBasicLrc() {
    std::printf("[1] 标准 LRC\n");
    const std::string text =
        "[ti:测试歌曲]\n"
        "[ar:测试艺术家]\n"
        "[00:01.50]第一行\n"
        "[00:12.34]第二行\n"
        "[01:02.99]第三行\n";

    const auto document = LyricParser::parse(text, "");
    CHECK(document.has_value());
    if (!document) return;

    CHECK(document->lines.size() == 3);
    CHECK(near(document->lines[0].startMs, 1500));
    CHECK(document->lines[0].text == "第一行");
    CHECK(near(document->lines[1].startMs, 12340));
    CHECK(document->lines[1].text == "第二行");
    CHECK(near(document->lines[2].startMs, 62990));
    CHECK(!document->hasTranslation);
}

// ── 2. 一行多时间戳 ────────────────────────────────────────────────────
void testMultipleTimestamps() {
    std::printf("[2] 一行多时间戳\n");
    const std::string text =
        "[00:10.00]副歌\n"
        "[00:30.00][01:10.00]重复的副歌\n";

    const auto document = LyricParser::parse(text, "");
    CHECK(document.has_value());
    if (!document) return;

    CHECK(document->lines.size() == 3);
    CHECK(near(document->lines[0].startMs, 10000));
    CHECK(near(document->lines[1].startMs, 30000));
    CHECK(near(document->lines[2].startMs, 70000));
    CHECK(document->lines[1].text == "重复的副歌");
    CHECK(document->lines[2].text == "重复的副歌");
}

// ── 3. 增强 LRC（逐字） ────────────────────────────────────────────────
void testEnhancedLrc() {
    std::printf("[3] 增强 LRC 逐字\n");
    const std::string text = "[00:05.00]<00:05.00>你<00:05.50>好<00:06.00>世界\n";

    const auto document = LyricParser::parse(text, "");
    CHECK(document.has_value());
    if (!document) return;

    CHECK(document->lines.size() == 1);
    const auto& line = document->lines[0];
    CHECK(line.text == "你好世界");
    CHECK(line.words.size() == 3);
    if (line.words.size() == 3) {
        CHECK(near(line.words[0].startMs, 5000));
        CHECK(near(line.words[0].endMs, 5500));
        CHECK(line.words[0].text == "你");
        CHECK(near(line.words[2].startMs, 6000));
        CHECK(line.words[2].text == "世界");
    }
}

// ── 4. 翻译合并 ────────────────────────────────────────────────────────
void testTranslation() {
    std::printf("[4] 翻译合并\n");
    const std::string original =
        "[00:01.00]Hello\n"
        "[00:03.00]World\n";
    // 第二条故意错开 120ms，验证容差配对
    const std::string translation =
        "[00:01.00]你好\n"
        "[00:03.12]世界\n";

    const auto document = LyricParser::parse(original, translation);
    CHECK(document.has_value());
    if (!document) return;

    CHECK(document->lines.size() == 2);
    CHECK(document->hasTranslation);
    CHECK(document->lines[0].translation == "你好");
    CHECK(document->lines[1].translation == "世界");
}

// ── 5. 脏数据 ──────────────────────────────────────────────────────────
void testJunkLines() {
    std::printf("[5] 脏数据（元数据/无时间行）\n");
    const std::string text =
        "[ti:标题]\n"
        "[by:某人]\n"
        "[offset:+500]\n"
        "这是一行没有时间戳的说明文字\n"
        "[00:02.00]有效行\n"
        "\n"
        "[ar:艺术家]\n";

    const auto document = LyricParser::parse(text, "");
    CHECK(document.has_value());
    if (!document) return;

    CHECK(document->lines.size() == 1);
    CHECK(document->lines[0].text == "有效行");
    CHECK(near(document->lines[0].startMs, 2000));
}

// ── 6. 空 / 无效输入 ───────────────────────────────────────────────────
void testEmptyInput() {
    std::printf("[6] 空输入\n");
    CHECK(!LyricParser::parse("", "").has_value());
    CHECK(!LyricParser::parse("[ti:只有元数据]\n", "").has_value());
    CHECK(!LyricParser::parse("纯文本没有时间轴", "").has_value());
}

// ── 7. 排序 ────────────────────────────────────────────────────────────
void testSorting() {
    std::printf("[7] 乱序输入要排好\n");
    const std::string text =
        "[00:30.00]后面的\n"
        "[00:10.00]前面的\n"
        "[00:20.00]中间的\n";

    const auto document = LyricParser::parse(text, "");
    CHECK(document.has_value());
    if (!document) return;

    CHECK(document->lines.size() == 3);
    CHECK(near(document->lines[0].startMs, 10000));
    CHECK(near(document->lines[1].startMs, 20000));
    CHECK(near(document->lines[2].startMs, 30000));
}

// ── 8. JSON 输出 ───────────────────────────────────────────────────────
void testJsonShape() {
    std::printf("[8] JSON 结构\n");
    const auto document = LyricParser::parse("[00:01.00]Hello\n", "[00:01.00]你好\n");
    CHECK(document.has_value());
    if (!document) return;

    const std::string json = document->toJson().dump();
    CHECK(json.find("\"hasTranslation\":true") != std::string::npos);
    CHECK(json.find("\"startMs\":1000") != std::string::npos);
    CHECK(json.find("\"text\":\"Hello\"") != std::string::npos);
    CHECK(json.find("\"translation\":\"你好\"") != std::string::npos);
}

} // namespace

int main() {
    std::printf("=== 歌词解析器测试 ===\n");

    testBasicLrc();
    testMultipleTimestamps();
    testEnhancedLrc();
    testTranslation();
    testJunkLines();
    testEmptyInput();
    testSorting();
    testJsonShape();

    std::printf("\n%d 项检查，%d 项失败\n", gChecks, gFailures);
    if (gFailures == 0) std::printf("结果: 全部通过\n");
    return gFailures == 0 ? 0 : 1;
}
