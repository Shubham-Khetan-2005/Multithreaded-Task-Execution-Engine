#include <iostream>
#include <chrono>
#include <vector>
#include "../include/TaskEngine.hpp"

int main() {
    const int num_tasks = 100000;
    std::cout << "Starting benchmark with " << num_tasks << " tasks...\n";

    // Initialize high-resolution clock for accurate telemetry
    auto start_time = std::chrono::high_resolution_clock::now();

    {
        // Dynamically scale the thread pool based on the host machine's hardware capabilities
        unsigned int hardware_threads = std::thread::hardware_concurrency();
        std::cout << "Booting engine with " << hardware_threads << " threads.\n";
        
        TaskEngine engine(hardware_threads);
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks); // Pre-allocate memory to prevent vector reallocation overhead

        // Blast the engine with concurrent math operations
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(engine.submit(1, [](int a, int b) {
                return a + b;
            }, i, i));
        }

        // Block the main thread until every single future has resolved
        for (auto& fut : futures) {
            fut.get(); 
        }
    } 

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    std::cout << "Successfully processed " << num_tasks << " tasks.\n";
    std::cout << "Total execution time: " << duration.count() << " ms.\n";

    return 0;
}