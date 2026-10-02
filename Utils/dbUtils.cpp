#include "./dbUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>

juce::String DbUtils::optStrCol(SQLite::Statement& sql, const char* colName) {
    auto col = sql.getColumn(colName);
    if (col.isNull()) return "";
    auto s = juce::String(col.getString());
    return s;
}
std::optional<int> DbUtils::optIntCol(SQLite::Statement& sql, const char* colName) {
    if (sql.getColumn(colName).isNull()) return std::nullopt;
    return sql.getColumn(colName).getInt();
}