/**
 * @brief 插件扫描的**纯逻辑**测试（PluginHost/PluginCategory.cpp + PluginScanProtocol.cpp）
 *
 * ── 测什么 ────────────────────────────────────────────────────────────
 *   1. 分类规则（规格 §2）的 6 条优先级，用真实插件的形态当样例：
 *        · Surge XT：2 进 2 出但自称音源 → 必须靠 isInstrument 兜住
 *        · TDR Nova：3 进 2 出（含侧链）→ 效果器
 *        · 监听/可视化插件：n 进 0 出 → 效果器
 *        · OB-Xd：元数据整条错掉（0/0）→ 其他，等用户手动改
 *   2. 优先级：isInstrument 必须压过"有输入就是效果器"（这条顺序是实测定的）
 *   3. category 匹配忽略大小写；type/rule 的字面量与数据库、前端约定一致
 *   4. 扫描线路协议：编码 → 解码往返，index/ordinal/type/rule/通道数/
 *      identifier/name/vendor/category 一个字段都不丢
 *   5. 协议的安全性：文本里的 \t \r \n 被替换成空格（否则一行会被劈成两行，
 *      主进程的 index 立刻错位）；不是本协议的行判成 unknown
 *
 * ── 通过标准 ──────────────────────────────────────────────────────────
 *   所有断言通过、退出码 0。任何一条不符打印 [FAIL] 并以退出码 1 结束。
 *
 * ── 为什么要测 ────────────────────────────────────────────────────────
 *   · 分类规则是整条扫描链上**唯一有主观判断**的地方，判错了整个插件库的分类
 *     就废了，而且"错了也不崩"，编译器不会提醒你，只能靠断言锁住。
 *   · 行协议是跨进程边界。解析错位会让扫描结果整体平移一个下标（插件 A 的数据
 *     写进 B 的行），这种 bug 在真机上极难查——它只在"某个插件扫失败"或者
 *     "插件名字里恰好有怪字符"的时候才出现。必须在纯函数层面钉死。
 *   · 两个文件都刻意不依赖 JUCE（见 PluginTypes.hpp 的说明），所以这个测试
 *     可以用裸 cl 编译，不需要 JUCE / SQLite / FFmpeg。
 *
 * ── 怎么跑（在 Test\ 目录下）──────────────────────────────────────────
 *   cl /utf-8 /std:c++20 /EHsc /I .. test_plugin_host.cpp ^
 *      ..\PluginHost\PluginCategory.cpp ..\PluginHost\PluginScanProtocol.cpp ^
 *      /Fe:test_plugin_host.exe
 *   .\test_plugin_host.exe
 */

#include "PluginHost/PluginCategory.hpp"
#include "PluginHost/PluginScanProtocol.hpp"

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

    using PluginHost::Category;
    using PluginHost::CategoryRule;

    /** @brief 断言一次分类结果，失败时把实际值打出来，省得反复猜 */
    void checkClassify(
        const char* label,
        bool isInstrument,
        const char* category,
        int numInputs,
        int numOutputs,
        Category expectedType,
        CategoryRule expectedRule
    ) {
        const auto decision = PluginHost::classify(
            isInstrument,
            category,
            numInputs,
            numOutputs
        );

        const bool ok = decision.type == expectedType && decision.rule == expectedRule;
        ++gChecks;
        if (ok) return;

        ++gFailures;
        std::printf(
            "  [FAIL] %s: 期望 %s(%s)，实际 %s(%s)  [%d进%d出, category=\"%s\"]\n",
            label,
            PluginHost::categoryToString(expectedType).c_str(),
            PluginHost::ruleToString(expectedRule).c_str(),
            PluginHost::categoryToString(decision.type).c_str(),
            PluginHost::ruleToString(decision.rule).c_str(),
            numInputs,
            numOutputs,
            category
        );
    }

    // ── 1. 分类规则 ────────────────────────────────────────────────────
    void testClassify() {
        std::printf("[1] 分类规则（规格 §2）\n");

        // 规则 1：元数据直接声明是音源。
        // Surge XT 是"带音频输入的音源"（2进2出），拓扑和效果器完全一样，
        // 只能靠这条拦住——这是本次实测后定下顺序的原因。
        checkClassify("Surge XT 形态", true, "Instrument|Synth", 2, 2,
                      Category::instrument, CategoryRule::isInstrument);
        checkClassify("OB-Xd 形态(0/0)", true, "", 0, 0,
                      Category::instrument, CategoryRule::isInstrument);

        // 规则 2：category 里含 Synth / Instrument（忽略大小写）
        checkClassify("category 带 Synth", false, "Synth", 2, 2,
                      Category::instrument, CategoryRule::categoryHint);
        checkClassify("category 小写 synth", false, "synth", 0, 0,
                      Category::instrument, CategoryRule::categoryHint);
        checkClassify("category 大写 INSTRUMENT", false, "INSTRUMENT", 0, 0,
                      Category::instrument, CategoryRule::categoryHint);

        // 规则 3：0 进 >0 出（生成器）
        checkClassify("0进2出", false, "", 0, 2,
                      Category::instrument, CategoryRule::noInput);

        // 规则 4：category 自称 Fx / Effect（只在元数据残缺的 0进0出 上生效）
        checkClassify("0进0出但自称 Fx", false, "Fx", 0, 0,
                      Category::effect, CategoryRule::categoryFx);
        checkClassify("0进0出但自称 Effect", false, "Effect", 0, 0,
                      Category::effect, CategoryRule::categoryFx);

        // 规则 5：有输入就是效果器
        // TDR Nova 报 3/2（2 主输入 + 1 侧链），通道数是总线总和，不是主输入个数
        checkClassify("TDR Nova 形态(3/2)", false, "Fx", 3, 2,
                      Category::effect, CategoryRule::categoryFx);
        checkClassify("普通立体声效果器", false, "", 2, 2,
                      Category::effect, CategoryRule::hasInput);
        // 监听 / 可视化插件是 n 进 0 出，也算效果器链的一种
        checkClassify("监听插件(2进0出)", false, "", 2, 0,
                      Category::effect, CategoryRule::hasInput);

        // 规则 6：0 进 0 出且毫无线索 → 兜底桶
        checkClassify("元数据全丢", false, "", 0, 0,
                      Category::other, CategoryRule::none);
        checkClassify("category 无关", false, "Analyzer", 0, 0,
                      Category::other, CategoryRule::none);

        // ── 优先级专项 ──
        // isInstrument 必须压过 category Fx 和"有输入"
        checkClassify("优先级: 音源自称 Fx", true, "Fx", 2, 2,
                      Category::instrument, CategoryRule::isInstrument);
        // 规则 3 必须压过规则 4：0进2出 就算自称 Fx 也是生成器
        checkClassify("优先级: 0进2出 + Fx", false, "Fx", 0, 2,
                      Category::instrument, CategoryRule::noInput);
        // 规则 1 压过规则 3：自称音源且 0进0出
        checkClassify("优先级: 音源 0进0出", true, "", 0, 0,
                      Category::instrument, CategoryRule::isInstrument);
    }

    // ── 2. 枚举与字面量 ────────────────────────────────────────────────
    void testCategoryStrings() {
        std::printf("[2] 枚举 ↔ 字面量\n");

        // 这三个字面量同时被数据库 plugins.type 和前端 JSON 使用，改了就炸前端
        CHECK(PluginHost::categoryToString(Category::instrument) == "instrument");
        CHECK(PluginHost::categoryToString(Category::effect) == "effect");
        CHECK(PluginHost::categoryToString(Category::other) == "other");

        Category parsed = Category::other;
        CHECK(PluginHost::categoryFromString("instrument", parsed) &&
              parsed == Category::instrument);
        CHECK(PluginHost::categoryFromString("effect", parsed) && parsed == Category::effect);
        CHECK(PluginHost::categoryFromString("other", parsed) && parsed == Category::other);

        // 大小写不宽容：前端传 "Effect" 应当被拒（不做隐式归一化，免得库里出现两种写法）
        CHECK(!PluginHost::categoryFromString("Effect", parsed));
        CHECK(!PluginHost::categoryFromString("", parsed));
        CHECK(!PluginHost::categoryFromString("fx", parsed));

        CHECK(PluginHost::scanStatusToString(PluginHost::ScanStatus::ok) == "ok");
        CHECK(PluginHost::scanStatusToString(PluginHost::ScanStatus::failed) == "failed");

        // 规则名只进日志，但也要稳定（出问题时靠它定位）
        CHECK(PluginHost::ruleToString(CategoryRule::isInstrument) == "isInstrument");
        CHECK(PluginHost::ruleToString(CategoryRule::categoryHint) == "category");
        CHECK(PluginHost::ruleToString(CategoryRule::noInput) == "no-input");
        CHECK(PluginHost::ruleToString(CategoryRule::categoryFx) == "category-fx");
        CHECK(PluginHost::ruleToString(CategoryRule::hasInput) == "has-input");
        CHECK(PluginHost::ruleToString(CategoryRule::none) == "none");
    }

    // ── 3. 协议往返 ────────────────────────────────────────────────────
    PluginHost::Record makeRecord() {
        PluginHost::Record record;
        record.ordinal = 2;
        record.type = Category::effect;
        record.rule = CategoryRule::hasInput;
        record.meta.name = "Pro-Q 4";
        record.meta.vendor = "FabFilter";
        record.meta.category = "Fx|EQ";
        record.meta.identifier = "VST3-Pro-Q4-1234";
        record.meta.numInputs = 3;
        record.meta.numOutputs = 2;
        record.meta.isInstrument = false;
        return record;
    }

    void testProtocolRoundTrip() {
        std::printf("[3] 协议往返\n");

        const auto record = makeRecord();
        const auto line = PluginHost::encodeOk(7, record);

        // 编码后必须只有一行（里面不能出现裸换行）
        CHECK(line.find('\n') == std::string::npos);
        CHECK(line.rfind("JFPLUG\tOK\t", 0) == 0);

        const auto event = PluginHost::decodeLine(line);
        CHECK(event.kind == PluginHost::ScanEvent::Kind::ok);
        CHECK(event.index == 7);
        CHECK(event.record.ordinal == 2);
        CHECK(event.record.type == Category::effect);
        CHECK(event.record.rule == CategoryRule::hasInput);
        CHECK(event.record.meta.numInputs == 3);
        CHECK(event.record.meta.numOutputs == 2);
        CHECK(event.record.meta.identifier == "VST3-Pro-Q4-1234");
        CHECK(event.record.meta.name == "Pro-Q 4");
        CHECK(event.record.meta.vendor == "FabFilter");
        CHECK(event.record.meta.category == "Fx|EQ");

        // END / FAIL / DONE / LOG
        const auto endLine = PluginHost::decodeLine(PluginHost::encodeEnd(7));
        CHECK(endLine.kind == PluginHost::ScanEvent::Kind::end);
        CHECK(endLine.index == 7);

        const auto failLine =
            PluginHost::decodeLine(PluginHost::encodeFail(11, "枚举抛出异常: access violation"));
        CHECK(failLine.kind == PluginHost::ScanEvent::Kind::fail);
        CHECK(failLine.index == 11);
        CHECK(failLine.reason == "枚举抛出异常: access violation");

        const auto doneLine = PluginHost::decodeLine(PluginHost::encodeDone(12));
        CHECK(doneLine.kind == PluginHost::ScanEvent::Kind::done);
        CHECK(doneLine.processed == 12);

        const auto logLine = PluginHost::decodeLine(PluginHost::encodeLog("实例化失败 X"));
        CHECK(logLine.kind == PluginHost::ScanEvent::Kind::log);
        CHECK(logLine.reason == "实例化失败 X");
    }

    // ── 4. 协议的安全性 ────────────────────────────────────────────────
    void testProtocolHardening() {
        std::printf("[4] 协议安全性（脏字符 / 噪声行）\n");

        // 插件名字里带制表符/换行时，必须被压成空格，否则一行变两行，
        // 主进程会把它当成两个事件，index 立刻错位。
        auto dirty = makeRecord();
        dirty.meta.name = "Pro\tQ\n4\rX";
        dirty.meta.vendor = "Fab\r\nFilter";

        const auto line = PluginHost::encodeOk(3, dirty);
        CHECK(line.find('\n') == std::string::npos);
        CHECK(line.find('\r') == std::string::npos);
        // 整个编码里只有协议本身的 11 个分隔制表符
        int tabs = 0;
        for (char ch : line) {
            if (ch == '\t') ++tabs;
        }
        CHECK(tabs == 11);

        const auto event = PluginHost::decodeLine(line);
        CHECK(event.kind == PluginHost::ScanEvent::Kind::ok);
        CHECK(event.index == 3);
        CHECK(event.record.meta.name == "Pro Q 4 X");
        CHECK(event.record.meta.vendor == "Fab  Filter");
        // 其它字段不受影响
        CHECK(event.record.meta.identifier == "VST3-Pro-Q4-1234");

        // sanitizeField 单独用（子进程拼日志行时也走它）
        CHECK(PluginHost::sanitizeField("a\tb") == "a b");
        CHECK(PluginHost::sanitizeField("a\nb") == "a b");
        CHECK(PluginHost::sanitizeField("正常中文") == "正常中文");

        // 第三方插件往进度文件里乱打的东西不能污染结果
        CHECK(PluginHost::decodeLine("").kind == PluginHost::ScanEvent::Kind::unknown);
        CHECK(PluginHost::decodeLine("hello world").kind ==
              PluginHost::ScanEvent::Kind::unknown);
        CHECK(PluginHost::decodeLine("JFPLUG").kind == PluginHost::ScanEvent::Kind::unknown);
        CHECK(PluginHost::decodeLine("JFPLUG\tOK").kind ==
              PluginHost::ScanEvent::Kind::unknown);
        CHECK(PluginHost::decodeLine("JFPLUG\tWHAT\t1\t2").kind ==
              PluginHost::ScanEvent::Kind::unknown);
        // 字段不够长（被截断的半行）也要判 unknown，不能让半条数据入库
        CHECK(PluginHost::decodeLine("JFPLUG\tOK\t1\t0\teffect\thas-input\t2\t2").kind ==
              PluginHost::ScanEvent::Kind::unknown);

        // index 非法时兜底成 -1（主进程会因此忽略这条，不会越界）
        CHECK(PluginHost::decodeLine("JFPLUG\tEND\tabc").index == -1);
    }

    // ── 5. 日志摘要 ────────────────────────────────────────────────────
    void testDescribeDecision() {
        std::printf("[5] 分类摘要（日志用）\n");

        const auto record = makeRecord();
        const auto text = PluginHost::describeDecision(
            record.meta,
            {record.type, record.rule}
        );
        CHECK(text == "effect(has-input) 3in/2out");
    }

} // namespace

int main() {
    std::printf("=== 插件扫描纯逻辑测试 ===\n");

    testClassify();
    testCategoryStrings();
    testProtocolRoundTrip();
    testProtocolHardening();
    testDescribeDecision();

    std::printf("=== %d 项断言，%d 项失败 ===\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
