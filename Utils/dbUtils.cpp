#include "./dbUtils.hpp"
#include "juce_core/juce_core.h"
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

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
// ── 可选 string 字段：字符串非空，无值则绑定 NULL ──
void DbUtils::bindOptStr(SQLite::Statement& stmt, const char* name, const juce::String& v) {
    if (v.isNotEmpty())
        stmt.bind(name, v.toStdString());
    else
        stmt.bind(name); // 无第二个参数 → SQL NULL
};
// ── 可选 int 字段 ──
void DbUtils::bindOptInt(SQLite::Statement& stmt, const char* name, const std::optional<int>& v) {
    if (v.has_value())
        stmt.bind(name, v.value());
    else
        stmt.bind(name);
};
void DbUtils::bindOptInt64(
    SQLite::Statement& stmt,
    const char* name,
    const std::optional<int64_t>& v
) {
    if (v.has_value())
        stmt.bind(name, v.value());
    else
        stmt.bind(name);
}