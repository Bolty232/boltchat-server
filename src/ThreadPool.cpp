#include "ThreadPool.h"
#include <stdexcept>
#include <iostream>

ThreadPool::ThreadPool(int threadCount)
    : running_(true), activeThreads_(0) {
    if (threadCount <= 0)
        throw std::invalid_argument("threadCount must be greater than 0.");

    for (int i = 0; i < threadCount; i++) {
        workers_.emplace_back([this] {
            this->workerLoop();
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    condition_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return !this->running_ || !this->tasks_.empty();
            });

            if (!this->running_ && this->tasks_.empty()) {
                return;
            }

            task = std::move(this->tasks_.front());
            this->tasks_.pop();
        }

        if (task) {
            activeThreads_++;
            try {
                task();
            }
            catch (const std::exception& e) {
                std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
            }
            catch (...) {
                std::cerr << "Unknown exception in thread pool task" << std::endl;
            }
            activeThreads_--;
        }
    }
}

bool ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return false;
        }
        if (tasks_.size() >= MAX_QUEUE_SIZE) {
            return false;
        }
        tasks_.push(std::move(task));
    }
    condition_.notify_one();
    return true;
}

size_t ThreadPool::getTaskCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

size_t ThreadPool::getActiveThreadCount() const {
    return activeThreads_.load();
}