#pragma once

#include "Utils/Yvar.hpp"
#include "Utils/constants.h"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Exception.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <cstring>
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
            } else if constexpr (std::is_same_v<T, juce::String> ||
                                 std::is_same_v<T, std::string>) {
                return result.getString();
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return result.getInt64();
            } else if constexpr (std::is_same_v<T, double>) {
                return result.getDouble();
            } else if constexpr (std::is_same_v<T, juce::StringArray>) {
                return Yvar{juce::JSON::fromString(result.getString())}.toStringArray();
            } else {
                return std::nullopt;
            }
        } catch (SQLite::Exception& e) {
            spdlog::get(LogDllID)->debug("getOptCol无法找到列名:{},错误信息:{}", colName, e.what());
            return std::nullopt;
        }
    }

    template <typename T>
    void bindOptValue(SQLite::Statement& stmt, const char* name, const std::optional<T>& v) {

        try {
            if constexpr (std::is_same_v<T, juce::String>) {
                if (v.has_value()) {
                    stmt.bind(name, v.value().toStdString());
                } else {
                    stmt.bind(name);
                }
            } else if constexpr (std::is_same_v<T, juce::StringArray>) {
                if (v.has_value()) {
                    stmt.bind(name, juce::JSON::toString(v.value()).toStdString());
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