#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <type_traits>
#include <stdexcept>

// Encapsulates a generic task with an assigned priority.
// The overloaded operator ensures higher priority tasks bubble to the top of our Max-Heap.
struct PriorityTask {
    int priority;
    std::function<void()> task;

    bool operator<(const PriorityTask& other) const {
        return this->priority < other.priority;
    }
};

class TaskEngine {
private:
    std::vector<std::thread> workers;
    std::priority_queue<PriorityTask> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop_flag;

    // The infinite lifecycle of a background worker.
    // Threads safely sleep here at 0% CPU utilization until work is assigned or a shutdown is triggered.
    void worker_loop() {
        while (true) {
            std::function<void()> current_task;

            {
                // Acquire lock before interacting with the shared queue
                std::unique_lock<std::mutex> lock(queue_mutex);
                
                // Sleep until the queue has tasks OR the engine is shutting down
                condition.wait(lock, [this] { return stop_flag || !tasks.empty(); });

                // Exit condition: Engine is stopping and all remaining tasks are cleared
                if (stop_flag && tasks.empty()) {
                    return;
                }

                current_task = std::move(tasks.top().task);
                tasks.pop();
            } 
            // Lock is automatically released here via RAII before task execution
            // This allows other threads to grab tasks while this thread is busy processing
            current_task();
        }
    }

public:
    // Initializes the thread pool and boots up the background workers immediately.
    TaskEngine(size_t num_threads) : stop_flag(false) {
        if (num_threads == 0) {
            throw std::invalid_argument("TaskEngine requires at least one thread.");
        }
        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back(&TaskEngine::worker_loop, this);
        }
    }

    // Safely drains the queue and joins all threads to prevent memory leaks or dangling processes.
    ~TaskEngine() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }
        
        // Wake all sleeping threads so they can evaluate the stop_flag
        condition.notify_all();
        
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Generic task submission interface.
    // Uses variadic templates and perfect forwarding to accept any callable object and its arguments.
    // Wraps the task in a packaged_task to extract a future, erases the type, and pushes to the queue.
    template<typename F, typename... Args>
    auto submit(int priority, F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        // Bind arguments to the function and wrap it to manage the return state
        auto task_wrapper = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result_future = task_wrapper->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop_flag) {
                throw std::runtime_error("Cannot submit tasks to a stopped TaskEngine.");
            }

            PriorityTask new_ticket;
            new_ticket.priority = priority;
            
            // Type Erasure: Hide the return type so it fits in the generic void() queue
            new_ticket.task = [task_wrapper]() {
                (*task_wrapper)();
            };

            tasks.push(std::move(new_ticket));
        }

        // Alert a single sleeping worker that a new task is ready
        condition.notify_one();
        return result_future;
    }
};