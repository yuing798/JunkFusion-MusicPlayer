#pragma once

#include "Utils/Yvar.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

namespace DbUtils {

    template <typename T> std::optional<T> getOptCol(SQLite::Statement& sql, const char* colName) {
        auto result{sql.getColumn(colName)};
        if (result.isNull()) return std::nullopt;
        if constexpr (std::is_same_v<T, int>) {
            return result.getInt();
        } else if constexpr (std::is_same_v<T, juce::String> || std::is_same_v<T, std::string>) {
            return result.getString();
        } else if constexpr (std::is_same_v<T, int64_t>) {
            return result.getInt64();
        } else if constexpr (std::is_same_v<T, double>) {
            return result.getDouble();
        } else if constexpr (std::is_same_v<T, juce::StringArray>) {
            return Yvar{juce::JSON::fromString(result.getString())}.toStringArray();
        }
        return std::nullopt;
    }

    template <typename T>
    void bindOptValue(SQLite::Statement& stmt, const char* name, const std::optional<T>& v) {

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
    }
} // namespace DbUtils