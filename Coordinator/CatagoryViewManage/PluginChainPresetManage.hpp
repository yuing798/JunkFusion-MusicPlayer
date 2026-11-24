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

    /** @brief [{id, title, createTime, preview}]，失败返回空数组
     *
     *  preview 是"TDR Nova → CHOWTapeModel"这种链预览，顺手在 SQL 循环里算好，
     *  省得前端为每个预设再发一次 FFI 去读配置体。
     */
    Yvar getAllPresets();

    /** @brief 取配置体（找不到返回空值） */
    Yvar getPresetSlots(int64_t id);

    /** @brief 新建；成功返回 id，失败 -1 */
    int64_t createPreset(const std::string& title, const std::string& slotsJson);

    /** @brief 覆盖已有预设的名字和配置体（双击预设改完保存） */
    void updatePreset(int64_t id, const std::string& title, const std::string& slotsJson);

    void renamePreset(int64_t id, const std::string& title);
    void deletePreset(int64_t id);

private:
    /// 侧边栏里最多显示几个插件名（再多就 "→ …"）
    static constexpr int kPreviewLimit = 4;

    /// 从配置体 JSON 里拼出 "A → B → C" 的预览串
    static std::string buildPreview(const std::string& slotsJson);

    CenterDbManage& mDbManage;
};
