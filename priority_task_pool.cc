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

namespace utils {

/// System monotonic timestamp
template <typename T> // Type of duration
inline int64_t STDTS()
{
    auto cur_tp =  steady_clock::now();
    auto dtn = cur_tp.time_since_epoch();
    return duration_cast<T>(dtn).count();
};

TaskUnit::TaskUnit()
  : task_(), priority_(0), timestamp_(STDTS<nanoseconds>())
{}

TaskUnit::TaskUnit(const Task& task, Priority priority)
  : task_(task), priority_(priority), timestamp_(STDTS<nanoseconds>())
{}

TaskUnit::TaskUnit(Task&& task, Priority priority)
  : task_(std::move(task)), priority_(priority), timestamp_(STDTS<nanoseconds>())
{}

TaskUnit::TaskUnit(const TaskUnit& other)
  : task_(other.task_), priority_(other.priority_), timestamp_(other.timestamp_)
{}

TaskUnit::TaskUnit(TaskUnit&& other)
  : task_(std::move(other.task_)), priority_(other.priority_), timestamp_(other.timestamp_)
{}

TaskUnit::~TaskUnit()
{}

TaskUnit& TaskUnit::operator=(const TaskUnit& other)
{
    if (this != &other) {
        task_ = other.task_;
        priority_ = other.priority_;
        timestamp_ = other.timestamp_;
    }
    return *this;
}

TaskUnit& TaskUnit::operator=(TaskUnit&& other)
{
    if (this != &other) {
        task_ = std::move(other.task_);
        priority_ = other.priority_;
        timestamp_ = other.timestamp_;
    }
    return *this;
}

bool TaskUnit::operator<(const TaskUnit& other) const
{
    if (priority_ == other.priority_)
        return timestamp_ > other.timestamp_; // FIFO
    else
        return priority_ < other.priority_;
}

void TaskUnit::operator()() const
{
    task_();
}

TaskPool::TaskPool(size_t threads) : stop_(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this]() {
            while(DoTask());
        });
    }
}

TaskPool::~TaskPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_.notify_all();
    for (std::thread& worker : workers_)
        worker.join();
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

}

