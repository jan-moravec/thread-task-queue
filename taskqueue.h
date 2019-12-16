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
class TaskQueueAbstract
{
public:
    TaskQueueAbstract() {}
    virtual ~TaskQueueAbstract() {}

    virtual void push(const T &data) = 0;
    virtual void push(T &&data) = 0;
    virtual void push(const std::vector<T> &data) = 0;
};

template<typename T>
class TaskQueue: public TaskQueueAbstract<T>
{
public:
    TaskQueue(std::function<void(const T &)> process): run{true}, process{process},
        thread{std::make_unique<std::thread>(&TaskQueue::loop, this)} {}
    ~TaskQueue() override
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
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(data);
        condition.notify_all();
    }
    void push(T &&data) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push_back(std::move(data));
        condition.notify_all();
    }
    void push(const std::vector<T> &data) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::copy(data.begin(), data.end(), std::back_inserter(queue));
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
        process.clear();

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

#endif // TASKQUEUE_H
