#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include <iostream>

#include <vector>
#include <initializer_list>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>

template<typename T>
class DataProcessQueueAbstract
{
public:
    DataProcessQueueAbstract() {}
    virtual ~DataProcessQueueAbstract() {}
    DataProcessQueueAbstract(const DataProcessQueueAbstract& rhs) = delete;
    DataProcessQueueAbstract& operator=(const DataProcessQueueAbstract& rhs) = delete;

    virtual void push(const T &data) = 0;
    virtual void push(T &&data) = 0;
    virtual void push(const std::vector<T> &data) = 0;
};

template<typename T>
class DataProcessQueue: public DataProcessQueueAbstract<T>
{
public:
    DataProcessQueue(std::function<void(const T &)> process): run{true}, process{process},
        thread{std::make_unique<std::thread>(&DataProcessQueue::loop, this)} {}
    ~DataProcessQueue() override
    {
        run = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            condition.notify_all();
        }
        thread->join();
    }

    void push(const T &data) override
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push_back(data);
        }
        condition.notify_all();
    }
    void push(T &&data) override
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push_back(std::move(data));
        }
        condition.notify_all();
    }
    void push(const std::vector<T> &data) override
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::copy(data.begin(), data.end(), std::back_inserter(queue));
        }
        condition.notify_all();
    }

private:
    void loop()
    {
        while (run && wait()) {
            std::vector<T> queue_process;
            while (load(queue_process)) {
                for (const T &data : queue_process) {
                    process(data);
                }

                queue_process.clear();
            }
        }
    }

    bool wait()
    {
        std::unique_lock<std::mutex> lck(mutex);
        if (!queue.size()) {
            condition.wait(lck);
        }
        return queue.size();
    }

    bool load(std::vector<T> &process)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.size()) {
            std::swap(queue, process);
            return true;
        }

        return false;
    }

private:
    std::vector<T> queue;
    std::atomic_bool run;
    std::unique_ptr<std::thread> thread;
    std::mutex mutex;
    std::condition_variable condition;
    std::function<void(const T &)> process;
};

class TaskQueue {

public:
    TaskQueue(std::size_t thread_count = 1): run{true}, threads{thread_count}
    {
        for(std::size_t i = 0; i < threads.size(); i++)
        {
            threads[i] = std::thread(&TaskQueue::loop, this);
        }
    }
    ~TaskQueue()
    {
        run = false;
        {
            std::lock_guard<std::mutex> lock(mutex);
            condition.notify_all();
        }

        for (std::thread &thread: threads)
        {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void push(const std::function<void(void)> &function)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push_back(function);
        }
        condition.notify_all();
    }
    void push(std::function<void(void)> &&function)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push_back(std::move(function));
        }
        condition.notify_all();
    }
    void push(const std::vector<std::function<void(void)>> &function)
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            std::copy(function.begin(), function.end(), std::back_inserter(queue));
        }
        condition.notify_all();
    }

private:
    void loop()
    {
        while (run && wait()) {
            std::vector<std::function<void(void)>> queue_process;
            while (load(queue_process)) {
                for (const std::function<void(void)> &function : queue_process) {
                    function();
                }

                queue_process.clear();
            }
        }
    }

    bool wait()
    {
        std::unique_lock<std::mutex> lck(mutex);
        if (!queue.size()) {
            condition.wait(lck);
        }
        return queue.size();
    }

    bool load(std::vector<std::function<void(void)>> &process)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.size()) {
            std::swap(queue, process);
            return true;
        }

        return false;
    }

private:
    std::vector<std::function<void(void)>> queue;
    std::atomic_bool run;
    std::mutex mutex;
    std::vector<std::thread> threads;
    std::condition_variable condition;
};

#endif // TASKQUEUE_H
