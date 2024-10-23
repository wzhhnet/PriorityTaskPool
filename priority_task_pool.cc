/*
 * Priority task pool
 * Implemented by C++
 *
 * Author wanch
 * Date 2024/7/15
 * Email wzhhnet@gmail.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include "priority_task_pool.h"

using namespace std::chrono;

namespace utils
{

/// System monotonic timestamp
template <typename T> // Type of duration
inline int64_t STDTS()
{
    auto cur_tp = steady_clock::now();
    auto dtn = cur_tp.time_since_epoch();
    return duration_cast<T>(dtn).count();
};

/// Task unit managed in the task pool
class TaskUnit final
{
  public:
    /// @brief constructor
    TaskUnit() : task_(), priority_(0), timestamp_(STDTS<nanoseconds>()) {}

    /// @brief constructor
    /// @param task lv-ref
    /// @param priority of task
    TaskUnit(const Task &task, Priority priority)
        : task_(task), priority_(priority), timestamp_(STDTS<nanoseconds>())
    {
    }

    /// @brief constructor
    /// @param task rv-ref
    /// @param priority of task
    TaskUnit(Task &&task, Priority priority)
        : task_(std::move(task)),
          priority_(priority),
          timestamp_(STDTS<nanoseconds>())
    {
    }

    /// @brief copy constructor
    /// @param other object lv-ref
    TaskUnit(const TaskUnit &other)
        : task_(other.task_),
          priority_(other.priority_),
          timestamp_(other.timestamp_)
    {
    }

    /// @brief move constructor
    /// @param other object rv-ref
    TaskUnit(TaskUnit &&other)
        : task_(std::move(other.task_)),
          priority_(other.priority_),
          timestamp_(other.timestamp_)
    {
    }

    /// @brief de-constructor
    ~TaskUnit() {}

    /// @brief copy assignment operator
    /// @param other object lv-ref
    /// @return this object
    TaskUnit &operator=(const TaskUnit &other)
    {
        if (this != &other) {
            task_ = other.task_;
            priority_ = other.priority_;
            timestamp_ = other.timestamp_;
        }
        return *this;
    }

    /// @brief move assignment operator
    /// @param other object rv-ref
    /// @return this object
    TaskUnit &operator=(TaskUnit &&other)
    {
        if (this != &other) {
            task_ = std::move(other.task_);
            priority_ = other.priority_;
            timestamp_ = other.timestamp_;
        }
        return *this;
    }

    /// @brief compare operator
    /// @param other object lv-ref
    /// @return true if other priority is higher than this(higher priority first)
    ///         true if this timestamp is greater than other(FIFO)
    bool operator<(const TaskUnit &other) const
    {
        if (priority_ == other.priority_)
            return timestamp_ > other.timestamp_; // FIFO
        else
            return priority_ < other.priority_;
    }

    /// @brief callable operator
    void operator()() const { task_(); }

  private:
    Task task_;
    Priority priority_;
    int64_t timestamp_;
};

TaskPool::TaskPool(size_t threads) : stop_(false)
{
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this]() { while (DoTask()); });
    }
}

TaskPool::~TaskPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_.notify_all();
    for (std::thread &worker : workers_) worker.join();
}

size_t TaskPool::size()
{
    std::unique_lock<std::mutex> lock(mutex_);
    return tasks_.size();
}

bool TaskPool::DoTask()
{
    TaskUnit task;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) return false;
        if (tasks_.empty()) {
            cond_.wait(lock); // waiting for task
            return true;
        } else {
            task = std::move(tasks_.top());
            tasks_.pop();
        }
    }
    task(); // Invoke task
    return true;
}

void TaskPool::AddTask(Task &&task, Priority pri)
{
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.emplace(std::forward<Task>(task), pri);
    cond_.notify_one();
}

} // namespace utils

