#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <random>
#include <mutex>
#include "../../include/TaskEngine.hpp"

// Thread-safe console logger to prevent interleaved text
std::mutex print_mutex;

template<typename... Args>
void safe_print(Args... args) {
    std::unique_lock<std::mutex> lock(print_mutex);
    (std::cout << ... << args) << "\n";
}

// Simulated network request function
// In a real application, this would use libcurl to make actual HTTP GET requests
std::string mock_http_get(const std::string& url) {
    safe_print("[Network] Establishing connection to: ", url, "...");

    // Simulate unpredictable network latency (100ms - 600ms)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(100, 600);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(distr(gen)));

    return "{ \"status\": 200, \"url\": \"" + url + "\", \"data\": \"mock_payload\" }";
}

int main() {
    safe_print("Initializing Concurrent Web Scraper Example...\n");

    // Start the total timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize the engine with a small pool of 4 threads
    TaskEngine engine(4);

    std::vector<std::string> target_urls = {
        "https://api.github.com/users",
        "https://api.stripe.com/v1/charges",
        "https://aws.amazon.com/ec2",
        "https://news.ycombinator.com",
        "https://leetcode.com/api/problems",
        "https://codeforces.com/api/contest.list"
    };

    std::vector<std::future<std::string>> download_futures;

    // Dispatch all URL fetch tasks concurrently
    for (const auto& url : target_urls) {
        // We assign priority 1 to all standard URLs
        int priority = 1;

        // Give a specific high-priority bump to Codeforces to show off the Max-Heap sorting
        if (url.find("codeforces") != std::string::npos) {
            priority = 10; 
            safe_print("[System] VIP URL detected. Escalating priority for: ", url);
        }

        download_futures.push_back(
            engine.submit(priority, mock_http_get, url)
        );
    }

    safe_print("\n[Main Thread] All scraping tasks dispatched. Waiting for network responses...\n");

    // Block the main thread and collect the resolved futures
    int success_count = 0;
    for (auto& fut : download_futures) {
        std::string json_response = fut.get();
        safe_print("[Scraper Result] Parsed: ", json_response);
        success_count++;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    safe_print("\n--- Scraping Summary ---");
    safe_print("Successfully scraped ", success_count, " URLs.");
    safe_print("Total Execution Time: ", duration.count(), " ms.");
    
    // Note: If executed sequentially, this would take up to ~3.6 seconds. 
    // Thanks to the TaskEngine, it completes in a fraction of that time.

    return 0;
}