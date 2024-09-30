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

#pragma once

#include <queue>
#include <thread>
#include <future>
#include <functional>
#include <condition_variable>

namespace utils {

using Task = std::function<void()>;
using Priority = uint8_t; // Greater number is higher priority

/// Task unit managed in the task pool
class TaskUnit final {
  public:
    TaskUnit();
    TaskUnit(const Task& task, Priority priority);
    TaskUnit(Task&& task, Priority priority);
    TaskUnit(const TaskUnit& other);
    TaskUnit(TaskUnit&& other);
    ~TaskUnit();
    TaskUnit& operator=(const TaskUnit& other);
    TaskUnit& operator=(TaskUnit&& other);
    bool operator<(const TaskUnit& other) const;
    void operator()() const;

  private:
    Task task_;
    Priority priority_;
    int64_t timestamp_;
};

class TaskPool final {
  public:
    TaskPool(size_t threads);
    ~TaskPool();
    size_t size();

    template <class F, class... Args>
    auto enqueue(Priority pri, F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        // typedef returning type
        using return_type = typename std::result_of<F(Args...)>::type;
        // bind type of F to task
        auto package = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = package->get_future();
        {
            Task task = [package]() -> void { (*package)(); };
            std::unique_lock<std::mutex> lock(mutex_);
            tasks_.emplace(std::move(task), pri);
        }
        cond_.notify_one();
        return res;
    }

private:
    bool DoTask();

private:
    using TaskQueue = std::priority_queue<TaskUnit>;
    std::vector<std::thread> workers_;
    TaskQueue tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool stop_;
};

}
