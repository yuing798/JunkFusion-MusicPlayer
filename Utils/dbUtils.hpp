#pragma once

#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <vector>

namespace DbUtils {

    template <typename T> std::optional<T> getOptCol(SQLite::Statement& sql, const char* colName) {
        try {
            auto result{sql.getColumn(colName)};
            if (result.isNull()) return std::nullopt;
            if (result.isInteger() && result.getInt64() == INT64T_MAX) return std::nullopt;
            if constexpr (std::is_same_v<T, int>) {
                return result.getInt();
            } else if constexpr (std::is_same_v<T, std::string>) {
                return result.getString();
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return result.getInt64();
            } else if constexpr (std::is_same_v<T, double>) {
                return result.getDouble();
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                return Yvar{nlohmann::json::parse(result.getString())}.toStringArray();
            } else {
                return std::nullopt;
            }
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug("getOptCol无法找到列名:{},错误信息:{}", colName, e.what());
            return std::nullopt;
        }
    }

    template <typename T1>
    void bindOptValue(SQLite::Statement& stmt, const char* name, const std::optional<T1>& v) {

        try {
            if constexpr (std::is_same_v<T1, std::vector<std::string>>) {
                if (v.has_value()) {
                    stmt.bind(name, nlohmann::json{v.value()}.dump());
                } else {
                    stmt.bind(name);
                }
            } else {
                if (v.has_value()) {
                    stmt.bind(name, v.value());
                } else {
                    stmt.bind(name);
                }
            }
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug("bindOptValue发生错误:列名{},错误信息:{}", name, e.what());
        }
    }
    template <typename T> std::vector<T> getBlob(SQLite::Statement& sql, const char* name) {
        try {
            const void* blob = sql.getColumn(name).getBlob();
            int size = sql.getColumn(name).getBytes();
            std::vector<T> buffer;
            buffer.resize(size / sizeof(T));
            std::memcpy(buffer.data(), blob, size);
            return buffer;
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug("getBlob发生错误:列名{},错误信息{}", name, e.what());
        }
    }

} // namespace DbUtils