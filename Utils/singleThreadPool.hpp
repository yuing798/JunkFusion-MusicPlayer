#pragma once

#include "juce_core/juce_core.h"
/**
 * @brief 单一任务线程池
 * 结合了 JUCE ThreadPool 和 C++ 标准库的 std::future/std::packaged_task
 */
class SingleThreadPool {
public:
    SingleThreadPool() = default;

    ~SingleThreadPool() {
        // 析构时清理队列，最多阻塞等待2秒以允许当前正在执行的任务安全退出
        pool.removeAllJobs(true, 2000);
    }

    // =======================================================
    // 提交普通任务 (即发即弃，无返回值)
    // =======================================================
    template <typename Func> void submit(Func&& task) {
        // JUCE 的 addJob 需要一个返回 JobStatus 的 Lambda
        // std::forward 是用来做完美转发的：把参数原封不动地传给下一层函数
        pool.addJob([t = std::forward<Func>(task)]() -> juce::ThreadPoolJob::JobStatus {
            t();
            return juce::ThreadPoolJob::jobHasFinished;
        });
    }

    // =======================================================
    // 2. 提交任务并要求异步返回结果
    // =======================================================
    template <typename Func> std::future<std::invoke_result_t<Func>> submitAsync(Func&& f) {
        // std::invoke_result_t:编译器推断调用某个可调用对象返回什么类型
        using ReturnType = std::invoke_result_t<Func>;

        /**
         * @explication
         * task 被 lambda 捕获；
         * lambda 被放进线程池队列；
         * 只要 lambda 还活着，shared_ptr 的引用计数就 > 0；
         * packaged_task 就还活着；
         * 任务执行完后 lambda 被销毁，引用计数归零，packaged_task 自动释放。
         *
         */
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<Func>(f));

        std::future<ReturnType> res = task->get_future();

        pool.addJob([task]() { (*task)(); });

        return res;
    }

private:
    juce::ThreadPool pool{1};
};