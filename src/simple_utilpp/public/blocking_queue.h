/**
 * blocking_queue.h
 */

#pragma once

#include <list>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <string>
#include <vector>
#include <queue>


typedef std::function<void()> TaskFunc;

class BlockingQueueTask
{
public:
    BlockingQueueTask(std::string name = "")
        : name_(name)
    {
    }

    ~BlockingQueueTask()
    {}

public:
    void Put(TaskFunc task)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.emplace_back(task);
        cond_.notify_one();
    }

    bool Start(uint32_t num)
    {
        stop_ = false;
        for (uint32_t i = 0; i < num; i++) {
            slaves_.emplace_back(std::thread([this] {
                while (!this->stop_) {
                    TaskFunc task;
                    {
                        std::unique_lock<std::mutex> lock(this->mutex_);
                        if (this->tasks_.empty()) {
                            this->cond_.wait(lock);
                        }

                        if (this->stop_)
                            break;

                        task = std::move(this->tasks_.front());
                        this->tasks_.pop_front();
                    }
                    task();
                }
            }));
        }
        return true;
    }

    void Stop()
    {
        stop_ = true;

        cond_.notify_all();
        for (auto& s : slaves_) {
            s.join();
        }

        slaves_.clear();
        tasks_.clear();
    }

private:
    std::string name_;
    std::vector<std::thread> slaves_;;
    std::list<TaskFunc> tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> stop_;
};




template <typename T>
class BlockingQueue
{
public:
    bool Empty()
    {
        return queue_.empty();
    }

    T Pop()
    {
        std::unique_lock<std::mutex> lock(mu_);
        cv_.wait(lock, [=, this] {return !this->queue_.empty(); });
        auto item = queue_.front();
        queue_.pop();
        return item;
    }

    void Push(T item)
    {
        std::unique_lock<std::mutex> lock(mu_);
        queue_.push(std::move(item));
        lock.unlock();
        cv_.notify_one();
    }

private:
    std::queue<T> queue_;
    std::mutex mu_;
    std::condition_variable cv_;
};
