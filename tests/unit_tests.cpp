#include <iostream>
#include <cassert>
#include <chrono>
#include <string>
#include <vector>
#include "../include/TaskEngine.hpp"

// Helper for clean test output
void run_test(const std::string& test_name, std::function<void()> test_func) {
    std::cout << "[TEST RUNNING] " << test_name << "...\n";
    test_func();
    std::cout << "[TEST PASSED]  " << test_name << "\n\n";
}

int main() {
    std::cout << "=== TaskEngine Unit Test Suite ===\n\n";

    run_test("Future Return Value Accuracy", []() {
        TaskEngine engine(2);
        
        // Submit a mathematical task
        auto future = engine.submit(1, [](int a, int b) {
            return a + b;
        }, 50, 50);
        
        // Assert that the engine correctly calculates and returns exactly 100
        assert(future.get() == 100 && "Math task failed to return the correct value.");
    });

    run_test("Max-Heap Priority Queue Sorting", []() {
        // We use 1 thread to force the queue to build up, proving it sorts correctly
        TaskEngine engine(1); 
        std::vector<int> execution_order;
        std::mutex tracker_mutex;

        // Block the single thread temporarily so the queue fills up
        auto blocker = engine.submit(100, []() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });

        // Submit tasks out of order
        engine.submit(1, [&]() { std::lock_guard<std::mutex> lock(tracker_mutex); execution_order.push_back(1); });
        engine.submit(5, [&]() { std::lock_guard<std::mutex> lock(tracker_mutex); execution_order.push_back(5); });
        engine.submit(2, [&]() { std::lock_guard<std::mutex> lock(tracker_mutex); execution_order.push_back(2); });

        // Wait for the blocker to finish, which frees the thread to process the queue
        blocker.get();

        // Let the queue drain
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

        // Assert that despite the submission order (1, 5, 2), they executed in priority order (5, 2, 1)
        std::lock_guard<std::mutex> lock(tracker_mutex);
        assert(execution_order.size() == 3 && "Not all tasks were executed.");
        assert(execution_order[0] == 5 && "Priority 5 did not execute first.");
        assert(execution_order[1] == 2 && "Priority 2 did not execute second.");
        assert(execution_order[2] == 1 && "Priority 1 did not execute third.");
    });

    run_test("Zero Thread Instantiation Exception", []() {
        bool exception_thrown = false;
        try {
            // Attempt to build an engine with 0 threads
            TaskEngine engine(0);
        } catch (const std::invalid_argument& e) {
            exception_thrown = true;
        }
        
        // Assert that our safety check inside the constructor successfully caught the bad input
        assert(exception_thrown && "Engine failed to throw exception when given 0 threads.");
    });

    std::cout << "=== All Unit Tests Executed Successfully ===\n";
    return 0;
}