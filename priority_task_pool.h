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
#if __cplusplus >= 201703L
#include <type_traits>
#endif
#include <condition_variable>

namespace utils
{

template <class F, class... Args>
#if __cplusplus >= 201703L
using ReturnType = typename std::invoke_result<F, Args...>::type;
#elif __cplusplus >= 201103L
using ReturnType = typename std::result_of<F(Args...)>::type;
#else
#error "c++11 or higher version must be supported"
#endif

using Task = std::function<void()>;
using Priority = uint8_t; // Greater number is higher priority
/// Forward Declaration
class TaskUnit;
/// @brief task pool class
class TaskPool final
{
  public:
    /// @brief constructor
    /// @param threads number of threads
    TaskPool(size_t threads);

    /// @brief de-constructor
    ~TaskPool();

    /// @brief number of pending tasks
    /// @return number of pending tasks
    size_t size();

    /// @brief add a callable object to task pool
    /// @tparam F type of callable object
    /// @tparam ...Args type of arguments list
    /// @param pri calling priority
    /// @param f callable object, maybe function, member function or lambda
    /// @param ...args  arguments list
    /// @return function returning value
    template <class F, class... Args>
    auto enqueue(Priority pri, F &&f,
		 Args &&...args) -> std::future<ReturnType<F, Args...>>
    {
        using Rtype = ReturnType<F, Args...>;
#if __cplusplus >= 202002L // C++20 Perfect forward by "pack init-capture"
        auto task = [f = std::forward<F>(f),
                     ... args = std::forward<Args>(args)]() mutable {
            AVX_RETURN_IF(true, std::invoke(f, std::forward<Args>(args)...),
                          AVX_VOID);
        };
#elif __cplusplus >= 201703L // C++17 Perfect forward by std::tuple
        auto task =
            [f = std::forward<F>(f),
             args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                AVX_RETURN_IF(true, std::apply(std::move(f), std::move(args)),
                              AVX_VOID);
            };
#else // C++11 Only copy args... type of rvalue-ref can not passed compiling.
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
#endif
        auto pkg =
            std::make_shared<std::packaged_task<Rtype()>>(std::move(task));
        std::future<Rtype> res = pkg->get_future();
        AddTask([pkg]() -> void { (*pkg)(); }, pri);
        return res;
    }

  private:
    /// @brief invoke pending task
    /// @return true if continue to invoke next pending task
    ///         false if task pool is stopped
    bool DoTask();

    /// @brief add a packed task to pending queue
    /// @param task packed task
    /// @param pri priority of task
    void AddTask(Task &&task, Priority pri);

  private:
    using TaskQueue = std::priority_queue<TaskUnit>;
    std::vector<std::thread> workers_;
    TaskQueue tasks_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool stop_;
};

} // namespace utils

