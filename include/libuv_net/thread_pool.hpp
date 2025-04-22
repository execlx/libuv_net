#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace libuv_net {

// 线程池类，用于管理线程和任务调度
class ThreadPool {
public:
    // 构造函数，默认使用硬件支持的线程数
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // 禁用拷贝构造和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 提交任务到线程池
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        
        // 创建任务
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        // 获取任务结果
        std::future<return_type> result = task->get_future();
        
        {
            // 加锁保护任务队列
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                throw std::runtime_error("线程池已停止，无法提交新任务");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        
        // 通知一个等待的线程
        condition_.notify_one();
        return result;
    }

    // 获取线程池大小
    size_t size() const { return threads_.size(); }
    // 检查线程池是否已停止
    bool is_stopped() const { return stop_; }

private:
    std::vector<std::thread> threads_; // 工作线程
    std::queue<std::function<void()>> tasks_; // 任务队列
    
    std::mutex queue_mutex_; // 任务队列互斥锁
    std::condition_variable condition_; // 条件变量
    std::atomic<bool> stop_{false}; // 停止标志
};

} // namespace libuv_net 