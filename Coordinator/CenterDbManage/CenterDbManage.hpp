#pragma once

#include "Utils/singleThreadPool.hpp"
#include <SQLiteCpp/SQLiteCpp.h>
#include <future>
#include <type_traits>

class CenterDbManage {
public:
    explicit CenterDbManage(const std::string& path);

    // =======================================================
    // 同步写：提交到写线程池，阻塞等待完成
    // 用法：db.write([](SQLite::Database& d) { ... });
    // =======================================================
    template <typename Func> auto write(Func&& f) -> std::invoke_result_t<Func, SQLite::Database&> {
        using ReturnType = std::invoke_result_t<Func, SQLite::Database&>;

        auto fut = pool.submitAsync([this, fn = std::forward<Func>(f)]() mutable -> ReturnType {
            return fn(db);
        });

        return fut.get(); // 阻塞，异常会在这里抛出
    }

    // =======================================================
    // 异步写：提交到写线程池，立即返回 future
    // 用法：auto fut = db.writeAsync([](SQLite::Database& d) { ... });
    // =======================================================
    template <typename Func>
    auto writeAsync(Func&& f) -> std::future<std::invoke_result_t<Func, SQLite::Database&>> {
        using ReturnType = std::invoke_result_t<Func, SQLite::Database&>;

        return pool.submitAsync([this, fn = std::forward<Func>(f)]() mutable -> ReturnType {
            return fn(db);
        });
    }

    // =======================================================
    // 读：可以直接在主线程读，不走写池
    // WAL 模式下读和写可以并发
    // =======================================================
    SQLite::Database& readDb() { return db; }

private:
    SQLite::Database db;
    SingleThreadPool pool;
};