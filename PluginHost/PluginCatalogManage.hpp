/**
 * @brief 插件库的数据库读写：`plugins` / `pluginDirs` 两张表（规格 §4）
 *
 * ── 谁在用 ──────────────────────────────────────────────────────────────
 *   只有 Coordinator（DLL）用。扫描子进程不碰数据库——它连 SQLite 都不链接，
 *   结果通过行协议回到 Coordinator，由这里统一入库（单一写入口，没有锁竞争）。
 *
 * ── 两条不能破的规则 ────────────────────────────────────────────────────
 *   1. **重扫不能覆盖用户改过的 type**：
 *      name / vendor / category / 通道数 这些是"元数据说了算"，每次扫描都刷新；
 *      type 是"用户说了算"（OB-Xd 这种元数据整个错掉的插件全靠人工纠正），
 *      upsert 的 DO UPDATE 里故意不写它。见 upsertOkSQL。
 *   2. **默认目录删不掉**：
 *      `C:\Program Files\Common Files\VST3` 这类系统目录由 JUCE/JUCE 同款约定
 *      提供，用户只能增删自己的目录。删除时用 isBuiltin 判断并返回原因。
 *
 * ── 目录被移除后插件行保留（规格 §4）────────────────────────────────────
 *   不级联删除。孤儿行留在表里，UI 汇总时按"当前目录列表"过滤即可——
 *   这样用户把目录加回来时插件立刻回来，不用重扫。
 *
 * ── 用法 ────────────────────────────────────────────────────────────────
 *     PluginCatalogManage catalog{dbContext.dbManage};
 *     catalog.ensureDefaultDirs();            // 启动时跑一次
 *     catalog.saveFileResult(filePath, records, nowMs);
 *     auto rows = catalog.getAll();
 */
#pragma once

#include "./PluginTypes.hpp"

#include "Coordinator/CenterDbManage/CenterDbManage.hpp"

#include <SQLiteCpp/Database.h>
#include <juce_core/juce_core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace PluginHost {

    class PluginCatalogManage {
    public:
        /** @brief plugins 表一行（原样给前端，字段名和前端 JSON 一致） */
        struct Row {
            std::string filePath;
            int ordinal{0};
            std::string identifier;
            std::string name;
            std::string vendor;
            std::string category;
            std::string type;
            int numInputs{0};
            int numOutputs{0};
            std::string scanStatus;
            std::string scanError;
            int64_t lastSeen{0};
        };

        struct DirRow {
            std::string path;
            bool isBuiltin{false};
        };

        explicit PluginCatalogManage(CenterDbManage& dbManage);

        // ── 目录 ──

        /**
         * @brief 把系统默认 VST3 目录写进表里（已存在则把 isBuiltin 刷成 1）
         *
         * 幂等，启动时无条件调一次。
         */
        void ensureDefaultDirs();

        std::vector<DirRow> getDirs();

        /** @brief 加一个用户目录；返回空串表示成功，否则是给用户看的原因 */
        std::string addDir(const std::string& path);

        /** @brief 删一个目录；默认目录会被拒绝 */
        std::string removeDir(const std::string& path);

        /** @brief 给扫描器用的现成目录列表（已过滤不存在的） */
        std::vector<juce::File> getExistingDirFiles();

        // ── 入库 ──

        /**
         * @brief 一个模块文件扫描成功后入库（可能一次写多行）
         *
         * type 不会被覆盖；上一次留下、这次没再出现的多余 ordinal 会被删掉。
         */
        void saveFileResult(
            const std::string& filePath,
            const std::vector<Record>& records,
            int64_t lastSeenMs
        );

        /** @brief 一个模块文件扫描失败：只更新状态，不动已入库的名字/类型 */
        void markFileFailed(
            const std::string& filePath,
            const std::string& error,
            int64_t lastSeenMs
        );

        // ── 查询 / 编辑 ──

        std::vector<Row> getAll();

        /** @brief 用户手动改 type（前端唯一允许改的字段） */
        bool setType(const std::string& identifier, const std::string& type);

    private:
        /** @brief 系统默认 VST3 目录（和 JUCE 的 VST3PluginFormatHeadless 保持一致） */
        static std::vector<std::string> defaultDirPaths();

        /**
         * @brief upsert 一条成功记录
         *
         * 注意：必须跑在 CenterDbManage 的写线程上（`db` 是那个线程的句柄），
         * 所以它是"接 SQLite::Database&"的内部函数，不自己去 submit。
         */
        static void upsertOk(
            SQLite::Database& db,
            const std::string& filePath,
            const Record& record,
            int64_t lastSeenMs
        );

        CenterDbManage& mDbManage;
    };

} // namespace PluginHost
