/**
 * @brief 插件黑名单 / 白名单（host-integration.md §6 的持久化部分）
 *
 * ── 两张表为什么不合并 ────────────────────────────────────────────────
 *   黑名单是**系统观测**（"这个插件把进程弄崩过 N 次"），只能由程序写；
 *   白名单是**用户意图**（"我知道它有问题，但我要用"），只能由用户操作写。
 *   合并成一张表 + 一个 bool，迟早会出现"重扫/升级把用户意图冲掉"的问题。
 *   规则见文档 §6.2：白名单优先级最高，不自动撤销，插件的白名单不因版本变化失效。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     view = getAll();                       // 前端要**全量显示**（不隐藏）
 *     isWhitelisted(identifier);
 *     getBlacklisted();                      // 下发链配置时用来过滤
 *     ban(identifier, filePath, "runtime-crash");
 *     clearBlack(identifier) / addWhite(...) / removeWhite(...);
 */
#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/Yvar.hpp"

#include <string>
#include <vector>

class PluginPolicyManage {
public:
    explicit PluginPolicyManage(CenterDbManage& dbManage) : mDbManage(dbManage) {}

    /** @brief {"blacklist":[{identifier,filePath,reason,failureCount,lastFailedAt}],
     *          "whitelist":[{identifier,filePath,note,createdAt}]} */
    Yvar getAll();

    /** @brief 黑名单里的 identifier 列表（下发链配置时过滤用） */
    std::vector<std::string> getBlacklisted();

    bool isWhitelisted(const std::string& identifier);

    /** @brief 记一次失败：已有则计数 +1 并刷新原因，没有则插入 */
    void ban(const std::string& identifier, const std::string& filePath, const std::string& reason);

    void clearBlack(const std::string& identifier);
    void addWhite(const std::string& identifier, const std::string& filePath);
    void removeWhite(const std::string& identifier);

private:
    CenterDbManage& mDbManage;
};
