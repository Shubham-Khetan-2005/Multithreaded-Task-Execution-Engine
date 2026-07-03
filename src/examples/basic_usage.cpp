#include <iostream>
#include <string>
#include <chrono>
#include "../../include/TaskEngine.hpp"
#include <mutex>

// A dedicated padlock just for the console megaphone
std::mutex print_mutex;

// Helper function to lock the console before printing
template<typename... Args>
void safe_print(Args... args) {
    std::unique_lock<std::mutex> lock(print_mutex);
    // C++17 fold expression to print all arguments seamlessly
    (std::cout << ... << args) << "\n";
}

int main() {
    safe_print("Initializing TaskEngine Basic Usage Example...\n");

    // Allocate a pool of 4 worker threads
    TaskEngine engine(4);

    // Submit a standard priority task that returns a string
    auto string_future = engine.submit(5, []() -> std::string {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return "System health check passed.";
    });

    // Submit a high priority task that returns an integer
    // This task will preempt lower priority tasks in the queue
    auto math_future = engine.submit(10, [](int a, int b) {
        return a * b;
    }, 21, 2);

    // Submit a low priority "fire-and-forget" void task
    engine.submit(1, []() {
        safe_print("[Background Thread] Logging telemetry data...");
    });

    safe_print("Main thread continues executing non-blocking operations...");

    // Block and retrieve the future states
    safe_print("Math Result (Priority 10): ", math_future.get());
    safe_print("String Result (Priority 5): ", string_future.get());

    return 0;
}