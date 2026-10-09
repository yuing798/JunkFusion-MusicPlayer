#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Statement.h>
#include <cstdint>
namespace DbUtils {
    juce::String optStrCol(SQLite::Statement& sql, const char* colName);

    std::optional<int> optIntCol(SQLite::Statement& sql, const char* colName);

    void bindOptStr(SQLite::Statement& stmt, const char* name, const juce::String& v);
    void bindOptInt(SQLite::Statement& stmt, const char* name, const std::optional<int>& v);
    void bindOptInt64(SQLite::Statement& stmt, const char* name, const std::optional<int64_t>& v);
} // namespace DbUtils