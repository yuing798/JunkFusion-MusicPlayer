#include "./PluginCatalogManage.hpp"

#include "Utils/constants.h"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Transaction.h>

#include <spdlog/spdlog.h>

namespace PluginHost {

    namespace {

        /**
         * 成功入库的 upsert。
         *
         * ⚠ type 那一行是这套 SQL 里唯一需要动脑的地方：
         *   默认**不覆盖** type——它是用户手动改的字段，重扫不许冲掉
         *   （规格 §4 / 验收标准 §6.5）。
         *   但有一个例外：上一次扫描**失败**过、留下的是占位行
         *   （type='other'，scanStatus='failed'），这次扫成功了必须把 type
         *   补上，否则那个插件永远卡在 other 里。
         *   所以用 CASE 把"上次是失败占位"这一种情况放行。
         *   （实测踩过：第一次扫描进程崩了，修好之后再扫，11 个插件 type 全是 other。）
         */
        constexpr const char* upsertOkSQL =
            "INSERT INTO plugins(filePath, ordinal, identifier, name, vendor, category, "
            "type, numInputs, numOutputs, scanStatus, scanError, lastSeen) "
            "VALUES(:filePath, :ordinal, :identifier, :name, :vendor, :category, "
            ":type, :in, :out, :status, NULL, :lastSeen) "
            "ON CONFLICT(filePath, ordinal) DO UPDATE SET "
            "identifier = excluded.identifier, "
            "name       = excluded.name, "
            "vendor     = excluded.vendor, "
            "category   = excluded.category, "
            "numInputs  = excluded.numInputs, "
            "numOutputs = excluded.numOutputs, "
            "scanStatus = excluded.scanStatus, "
            "scanError  = NULL, "
            "lastSeen   = excluded.lastSeen, "
            "type       = CASE WHEN plugins.scanStatus = 'failed' "
            "                  THEN excluded.type ELSE plugins.type END";

        /**
         * 失败标记：**只在没有行的时候插入占位行**，有行就只更新状态。
         * 不然一个"以前扫成功、这次扫失败"的插件会被文件名顶掉真实名字。
         */
        constexpr const char* upsertFailSQL =
            "INSERT INTO plugins(filePath, ordinal, identifier, name, vendor, category, "
            "type, numInputs, numOutputs, scanStatus, scanError, lastSeen) "
            "VALUES(:filePath, 0, '', :name, NULL, NULL, 'other', 0, 0, 'failed', :error, :lastSeen) "
            "ON CONFLICT(filePath, ordinal) DO UPDATE SET "
            "scanStatus = 'failed', "
            "scanError  = :error, "
            "lastSeen   = :lastSeen";

    } // namespace

    PluginCatalogManage::PluginCatalogManage(CenterDbManage& dbManage) : mDbManage(dbManage) {}

    std::vector<std::string> PluginCatalogManage::defaultDirPaths() {
        // 和 juce_audio_processors_headless 里 VST3PluginFormatHeadless::getDefaultLocationsToSearch()
        // 的取值一致。这里硬编码是有意的：Coordinator 不应该为了问一句"默认目录在哪"
        // 就把整个 VST3 SDK 链接进来。
#if JUCE_WINDOWS
        return {R"(C:\Program Files\Common Files\VST3)"};
#elif JUCE_MAC
        return {"/Library/Audio/Plug-Ins/VST3"};
#else
        return {"/usr/lib/vst3", "/usr/local/lib/vst3"};
#endif
    }

    void PluginCatalogManage::ensureDefaultDirs() {
        const auto defaults = defaultDirPaths();

        try {
            mDbManage.write([&](SQLite::Database& db) {
                SQLite::Statement sql{
                    db,
                    "INSERT INTO pluginDirs(path, isBuiltin) VALUES(:path, 1) "
                    "ON CONFLICT(path) DO UPDATE SET isBuiltin = 1"
                };
                for (const auto& path : defaults) {
                    sql.bind(":path", path);
                    sql.exec();
                    sql.reset();
                    sql.clearBindings();
                }
            });
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::ensureDefaultDirs 失败:{}", e.what());
        }
    }

    std::vector<PluginCatalogManage::DirRow> PluginCatalogManage::getDirs() {
        std::vector<DirRow> rows;

        try {
            SQLite::Statement sql{
                mDbManage.readDb(),
                "SELECT path, isBuiltin FROM pluginDirs ORDER BY isBuiltin DESC, path ASC"
            };
            while (sql.executeStep()) {
                DirRow row;
                row.path = sql.getColumn("path").getString();
                row.isBuiltin = sql.getColumn("isBuiltin").getInt() != 0;
                rows.push_back(std::move(row));
            }
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::getDirs 失败:{}", e.what());
        }

        return rows;
    }

    std::string PluginCatalogManage::addDir(const std::string& path) {
        // fromUTF8 只吃 const char*（C++20 里还有 char8_t*），std::string 得自己给 c_str
        const juce::File dir{juce::String::fromUTF8(path.c_str())};
        if (!dir.isDirectory()) return "目录不存在";

        // 统一存绝对路径，避免"./VST3"和"C:\...\VST3"变成两行
        const auto fullPath = dir.getFullPathName().toStdString();

        try {
            mDbManage.write([&](SQLite::Database& db) {
                SQLite::Statement sql{
                    db,
                    "INSERT INTO pluginDirs(path, isBuiltin) VALUES(:path, 0) "
                    "ON CONFLICT(path) DO NOTHING"
                };
                sql.bind(":path", fullPath);
                sql.exec();
            });
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::addDir 失败:{}", e.what());
            return "写入数据库失败";
        }

        return {};
    }

    std::string PluginCatalogManage::removeDir(const std::string& path) {
        const juce::File dir{juce::String::fromUTF8(path.c_str())};
        const auto fullPath = dir.getFullPathName().toStdString();

        try {
            bool isBuiltin = false;
            {
                SQLite::Statement sql{
                    mDbManage.readDb(),
                    "SELECT isBuiltin FROM pluginDirs WHERE path = :path"
                };
                sql.bind(":path", fullPath);
                if (!sql.executeStep()) return "目录不在列表里";
                isBuiltin = sql.getColumn("isBuiltin").getInt() != 0;
            }

            // 默认目录不允许删除（规格 §4）。用户可以加自己的目录，但不能把
            // 系统 VST3 目录从扫描范围里摘掉——否则"插件的插件为什么不见了"
            // 这种问题会变成客服问题。
            if (isBuiltin) return "系统默认目录不能删除";

            mDbManage.write([&](SQLite::Database& db) {
                SQLite::Statement sql{db, "DELETE FROM pluginDirs WHERE path = :path"};
                sql.bind(":path", fullPath);
                sql.exec();
            });
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::removeDir 失败:{}", e.what());
            return "操作数据库失败";
        }

        return {};
    }

    std::vector<juce::File> PluginCatalogManage::getExistingDirFiles() {
        std::vector<juce::File> dirs;
        for (const auto& row : getDirs()) {
            const juce::File dir{juce::String::fromUTF8(row.path.c_str())};
            if (dir.isDirectory()) dirs.push_back(dir);
        }
        return dirs;
    }

    void PluginCatalogManage::upsertOk(
        SQLite::Database& db,
        const std::string& filePath,
        const Record& record,
        int64_t lastSeenMs
    ) {
        SQLite::Statement sql{db, upsertOkSQL};
        sql.bind(":filePath", filePath);
        sql.bind(":ordinal", record.ordinal);
        sql.bind(":identifier", record.meta.identifier);
        sql.bind(":name", record.meta.name);
        sql.bind(":vendor", record.meta.vendor);
        sql.bind(":category", record.meta.category);
        sql.bind(":type", categoryToString(record.type));
        sql.bind(":in", record.meta.numInputs);
        sql.bind(":out", record.meta.numOutputs);
        sql.bind(":status", scanStatusToString(ScanStatus::ok));
        sql.bind(":lastSeen", static_cast<int64_t>(lastSeenMs));
        sql.exec();
    }

    void PluginCatalogManage::saveFileResult(
        const std::string& filePath,
        const std::vector<Record>& records,
        int64_t lastSeenMs
    ) {
        if (filePath.empty()) return;

        try {
            // 整个文件的所有行必须原子落库，而且要**在同一条写线程的同一个事务里**
            // 完成：SQLite 连接不是线程安全的，事务对象和实际执行必须在同一个线程。
            mDbManage.write([&](SQLite::Database& db) {
                SQLite::Transaction transaction{db};

                for (const auto& record : records) {
                    upsertOk(db, filePath, record, lastSeenMs);
                }

                // 上次是 2 个类型、这次只剩 1 个：把多余的 ordinal 清掉
                {
                    SQLite::Statement sql{
                        db,
                        "DELETE FROM plugins WHERE filePath = :filePath AND ordinal >= :keep"
                    };
                    sql.bind(":filePath", filePath);
                    sql.bind(":keep", static_cast<int>(records.size()));
                    sql.exec();
                }

                transaction.commit();
            });
        } catch (const std::exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::saveFileResult 失败 {}:{}", filePath, e.what());
        }
    }

    void PluginCatalogManage::markFileFailed(
        const std::string& filePath,
        const std::string& error,
        int64_t lastSeenMs
    ) {
        if (filePath.empty()) return;

        try {
            mDbManage.write([&](SQLite::Database& db) {
                SQLite::Statement sql{db, upsertFailSQL};
                sql.bind(":filePath", filePath);
                sql.bind(":name", juce::File{juce::String::fromUTF8(filePath.c_str())}
                                    .getFileNameWithoutExtension()
                                    .toStdString());
                sql.bind(":error", error);
                sql.bind(":lastSeen", static_cast<int64_t>(lastSeenMs));
                sql.exec();
            });
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error(
                    "PluginCatalogManage::markFileFailed 失败 {}:{}",
                    filePath,
                    e.what()
                );
        }
    }

    std::vector<PluginCatalogManage::Row> PluginCatalogManage::getAll() {
        std::vector<Row> rows;

        try {
            SQLite::Statement sql{
                mDbManage.readDb(),
                "SELECT filePath, ordinal, identifier, name, vendor, category, type, "
                "numInputs, numOutputs, scanStatus, scanError, lastSeen "
                "FROM plugins ORDER BY name COLLATE NOCASE ASC, filePath ASC"
            };
            while (sql.executeStep()) {
                Row row;
                row.filePath = sql.getColumn("filePath").getString();
                row.ordinal = sql.getColumn("ordinal").getInt();
                row.identifier = sql.getColumn("identifier").getString();
                row.name = sql.getColumn("name").getString();
                row.vendor = sql.getColumn("vendor").getString();
                row.category = sql.getColumn("category").getString();
                row.type = sql.getColumn("type").getString();
                row.numInputs = sql.getColumn("numInputs").getInt();
                row.numOutputs = sql.getColumn("numOutputs").getInt();
                row.scanStatus = sql.getColumn("scanStatus").getString();
                row.scanError = sql.getColumn("scanError").getString();
                row.lastSeen = sql.getColumn("lastSeen").getInt64();
                rows.push_back(std::move(row));
            }
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::getAll 失败:{}", e.what());
        }

        return rows;
    }

    bool PluginCatalogManage::setType(
        const std::string& identifier,
        const std::string& type
    ) {
        Category category;
        if (!categoryFromString(type, category)) return false;

        try {
            mDbManage.write([&](SQLite::Database& db) {
                SQLite::Statement sql{
                    db,
                    "UPDATE plugins SET type = :type WHERE identifier = :identifier"
                };
                sql.bind(":type", categoryToString(category));
                sql.bind(":identifier", identifier);
                sql.exec();
            });
            return true;
        } catch (const SQLite::Exception& e) {
            if (auto logger = spdlog::get(LogDllID))
                logger->error("PluginCatalogManage::setType 失败:{}", e.what());
            return false;
        }
    }

} // namespace PluginHost
