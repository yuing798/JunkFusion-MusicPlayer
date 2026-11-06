#pragma once

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"
#include "Utils/Yvar.hpp"
#include <cstdint>
#include <string>

/**
 * @brief EQ 预设的增删改查
 *
 * ── 文件意义 ──────────────────────────────────────────────────────────
 * 用户保存的 EQ 预设存在 SQLite 的 eqPresets 表里。表里只有三样东西：
 * id、标题、参数体 JSON。**参数体就是 EqWire::encodeAll 的输出**，
 * 和 getEqAllState() 内层完全同构，所以"读预设 -> 下发"不需要任何转换。
 *
 * 本类只管存取，不碰"当前 EQ 状态"（那是 EqController 的事），也不负责下发。
 *
 * ── 用法 ─────────────────────────────────────────────────────────────
 *     eqPresetManage.getAllPresets();                     // 列表页
 *     eqPresetManage.getPresetParams(id);                 // 取参数体
 *     eqPresetManage.createPreset("流行", paramsText);    // 保存当前 EQ
 *     eqPresetManage.renamePreset(id, "新名字");
 *     eqPresetManage.deletePreset(id);
 */
class EqPresetManage {
private:
    CenterDbManage& dbManage;

public:
    explicit EqPresetManage(CenterDbManage& dbManage) : dbManage(dbManage) {}

    /**
     * @brief 所有预设的视图（不含参数体，列表页用）
     *
     * @return [{id, title, createTime}]，查询失败返回空数组（不抛异常）
     */
    Yvar getAllPresets();

    /**
     * @brief 取某个预设的参数体
     *
     * @return 形状与 EqWire::encodeAll 一致；找不到时返回空值（isVoid() == true）
     */
    Yvar getPresetParams(int64_t id);

    /**
     * @brief 新建预设
     *
     * @param paramsText EqWire::encodeAll(...).dump() 的结果
     * @return 新预设的 id；失败返回 -1
     */
    int64_t createPreset(const std::string& title, const std::string& paramsText);

    void renamePreset(int64_t id, const std::string& title);
    void deletePreset(int64_t id);
};
