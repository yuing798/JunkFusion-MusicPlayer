/**
 * @brief 全局效果器链预设的增删改查
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * 和 EQ 预设完全一个套路：表里只有 id / 标题 / 创建时间 / **配置体 JSON**。
 * 配置体就是 `PluginChainController::apply()` 吃的那个形状：
 *
 *     {"bypassAll":false,
 *      "slots":[{"index":0,"filePath":"...","identifier":"...",
 *                "bypass":false,"state":"<base64>"}, ...]}
 *
 * 本类只管存取，不碰"当前链"（那是 PluginChainController 的事），也不负责下发。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     getAllPresets();                          // 预设列表（不含配置体）
 *     getPresetSlots(id);                       // 取配置体，直接喂给 apply
 *     createPreset("我的母带链", slotsJson);     // 保存当前链
 *     renamePreset(id, "新名字");
 *     deletePreset(id);
 */
#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/Yvar.hpp"
#include <cstdint>
#include <string>

class PluginChainPresetManage {
public:
    explicit PluginChainPresetManage(CenterDbManage& dbManage) : mDbManage(dbManage) {}

    /** @brief [{id, title, createTime}]，失败返回空数组 */
    Yvar getAllPresets();

    /** @brief 取配置体（找不到返回空值） */
    Yvar getPresetSlots(int64_t id);

    /** @brief 新建；成功返回 id，失败 -1 */
    int64_t createPreset(const std::string& title, const std::string& slotsJson);

    void renamePreset(int64_t id, const std::string& title);
    void deletePreset(int64_t id);

private:
    CenterDbManage& mDbManage;
};
