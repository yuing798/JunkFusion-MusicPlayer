/**
 * @brief 扫描子进程 ↔ 主进程之间的**行协议**（一行一个事件，UTF-8）
 *
 * ── 为什么要有这个东西 ──────────────────────────────────────────────────
 * 批量扫描必须在独立进程里跑（坏插件在枚举阶段就能把进程带走），于是
 * "子进程扫到了什么"必须跨进程传给主进程。规格 §3.3 要求**每扫完一个就
 * 立刻上报一行**，主进程才能实现"崩了就从这个 index 继续"，并且知道该把
 * 哪个插件标记为失败。
 *
 * ── 为什么不用 JSON ─────────────────────────────────────────────────────
 *   1. 扫描子进程要加载任意第三方 DLL，依赖越少越不容易被带崩；用 JSON 就得
 *      引入 nlohmann/Yvar，等于把整个 Utils（含 SQLite）拖进子进程。
 *   2. 字段是固定的十来个标量，回车换行本身就能当分隔符，解析不会更脆弱。
 *   3. 主进程**已经知道**每个 index 对应哪个文件（列表是自己写的），所以
 *      路径根本不需要传输，顺便消灭了"路径里带制表符"这个边角问题。
 *
 * ── 行格式（TAB = \t，所有文本字段里的 \t \r \n 在编码时被替换成空格）──
 *
 *   JFPLUG \t OK    \t <index> \t <ordinal> \t <type> \t <rule> \t <in> \t <out>
 *                              \t <identifier> \t <name> \t <vendor> \t <category>
 *   JFPLUG \t END   \t <index>          -- 这个 index 扫完了（至少产出一条 OK）
 *   JFPLUG \t FAIL  \t <index> \t <reason>
 *   JFPLUG \t DONE  \t <processed>
 *   JFPLUG \t LOG   \t <message>
 *
 *   前缀 JFPLUG 是必须的：第三方插件会往 stdout 乱打东西，主进程只认带前缀
 *   的行，其余一律当噪声丢掉（避免"插件打了一行字 → 扫描结果错位"）。
 *
 *   为什么需要 END：一个模块文件可能导出多个插件类型，主进程没法从"又收到一条
 *   OK"判断上一个文件到底扫完没有（尤其崩溃会发生在两个文件之间）。
 *   END 把"这个下标成功结束"变成显式事件，主进程的断点续扫只需要认它和 FAIL。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *   子进程：out.append(PluginHost::encodeOk(index, record));
 *   主进程：auto e = PluginHost::decodeLine(line);
 *           if (e.kind == ScanEvent::Kind::ok) { ... e.index / e.record ... }
 */
#pragma once

#include "./PluginTypes.hpp"

#include <string>
#include <string_view>

namespace PluginHost {

    /** @brief 魔数前缀，见上 */
    inline constexpr const char* kProtocolPrefix = "JFPLUG";

    struct ScanEvent {
        enum class Kind {
            ok,      // 一个插件类型扫完了
            end,     // 这个 index（模块文件）成功结束
            fail,    // 这个模块文件扫失败（枚举抛异常 / 一个类型都没拿到）
            done,    // 本批次正常结束
            log,     // 子进程的诊断日志，主进程转发到自己的日志
            unknown  // 不是本协议的行（第三方插件的噪声），直接丢掉
        };

        Kind kind{Kind::unknown};
        int index{-1};        // 对应主进程文件列表里的下标
        int processed{0};     // kind==done：本批次处理了多少个
        Record record;        // kind==ok
        std::string reason;   // kind==fail / kind==log
    };

    /** @brief 编码：一个成功的插件类型 */
    std::string encodeOk(int index, const Record& record);

    /** @brief 编码：这个模块文件成功结束 */
    std::string encodeEnd(int index);

    /** @brief 编码：一个模块文件整体失败 */
    std::string encodeFail(int index, std::string_view reason);

    /** @brief 编码：本批次正常结束 */
    std::string encodeDone(int processed);

    /** @brief 编码：子进程诊断日志 */
    std::string encodeLog(std::string_view message);

    /** @brief 解码一行（不含行尾 \r\n）；不认识的行返回 Kind::unknown */
    ScanEvent decodeLine(std::string_view line);

    /** @brief 把文本字段里的 \t \r \n 换成空格（编码时内部调用，测试也用它） */
    std::string sanitizeField(std::string_view text);

} // namespace PluginHost
