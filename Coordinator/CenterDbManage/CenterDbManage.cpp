#include "./CenterDbManage.hpp"

CenterDbManage::CenterDbManage(const std::string& path)
    : db(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {

    db.exec("PRAGMA busy_timeout = 5000;");
    db.exec("PRAGMA foreign_keys = ON;");
    db.exec("PRAGMA synchronous = NORMAL;");

    db.exec("PRAGMA journal_mode = WAL;");
}