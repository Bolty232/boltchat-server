#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(int threadCount);
    ~ThreadPool();

    bool enqueue(std::function<void()> task);

    size_t getTaskCount() const;
    size_t getActiveThreadCount() const;

private:
    void workerLoop();

    static constexpr size_t MAX_QUEUE_SIZE = 5000;

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::atomic<size_t> activeThreads_;
    bool running_;

    mutable std::mutex mutex_;
    std::condition_variable condition_;
};

#endif //THREADPOOL_H