#include "./OnlineGetMatedata.hpp"
#include "../dllUtils.hpp"
#include "constants.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <SQLiteCpp/Database.h>
#include <cpr/cpr.h>
#include <cpr/response.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

OnlineGetMatedata::OnlineGetMatedata(SQLite::Database& db)
    : juce::Thread("OnlineGetMatedata"), db(db) {}

void OnlineGetMatedata::setTask(OnlineGetMatedata::Task task) {
    std::lock_guard<std::mutex> lock{mtx};
    mTaskQueue.push(std::move(task));
    notify();
    if (!isThreadRunning()) startThread();
}
void OnlineGetMatedata::run() {
    while (!threadShouldExit()) {
        Task task;
        bool hasTask = false;

        // 1. 缩小锁的作用域，仅在弹出任务时持锁
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!mTaskQueue.empty()) {
                task = std::move(mTaskQueue.front());
                mTaskQueue.pop();
                hasTask = true;
            }
        } // 锁在此处自动释放

        // 2. 根据是否有任务决定处理还是休眠
        if (hasTask) {
            searchDataByPrint(std::move(task)); // 在锁外执行耗时任务
        } else {
            // 在【无锁状态】下安全挂起等待新任务！
            wait(-1);
        }
    }
}

void OnlineGetMatedata::searchDataByPrint(OnlineGetMatedata::Task task) {
    std::string client_key = "tvFS66WzNc";

    // 发起 POST 请求
    cpr::Response res = cpr::Post(
        cpr::Url{"https://api.acoustid.org/v2/lookup"},
        // cpr::Payload 会自动将参数放在 HTTP Body 中以 POST 表单发出
        cpr::Payload{
            {"client", client_key},
            {"meta", "recordings+releases+tracks"}, // 一次性拿全元数据
            {"duration", std::to_string(task.duration)},
            {"fingerprint", task.print},
            {"format", "json"}
        },
        cpr::Timeout{5000} // 5秒超时
    );

    if (res.status_code == 200) {
        // std::cout << "请求成功！返回 JSON 数据：" << std::endl;
        spdlog::get(LogDllID)->debug("得到网络请求返回的表单数据:{}", res.text);

    } else {
        spdlog::get(LogDllID)->debug("请求失败，错误码:{}", res.status_code);
    }
}