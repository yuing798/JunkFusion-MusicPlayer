#pragma once

#include "juce_core/juce_core.h"
#include <SQLiteCpp/Statement.h>
namespace DbUtils {
    juce::String optStrCol(SQLite::Statement& sql, const char* colName);

    std::optional<int> optIntCol(SQLite::Statement& sql, const char* colName);
} // namespace DbUtils