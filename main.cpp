#include <iostream>
#include <vector>
#include <thread>
#include <chrono> 
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class TaskEngine {
private:
    // The container for our worker threads
    std::vector<std::thread> workers;

    // The Queue: Stores simple void functions
    std::queue<std::function<void()>> tasks;
    
    // The Synchronization Primitives
    std::mutex queue_mutex;
    std::condition_variable condition;
    
    // A thread-safe boolean to tell workers when to exit their infinite loops
    bool stop_flag;

    // The infinite loop every thread will run
    void worker_loop(int thread_id) {
        std::cout << "Worker " << thread_id << " is online and sleeping...\n";
        
        while (true) {
            std::function<void()> task;
            
            { // --- CRITICAL SECTION START ---
                std::unique_lock<std::mutex> lock(queue_mutex);
                
                // The thread goes to sleep right here! 
                // It only wakes up IF: stop_flag is true OR tasks queue is not empty
                condition.wait(lock, [this] { 
                    return stop_flag || !tasks.empty(); 
                });

                // If the engine is shutting down and all tasks are done, break the loop
                if (stop_flag && tasks.empty()) {
                    std::cout << "Worker " << thread_id << " shutting down.\n";
                    return; 
                }

                // Grab the task off the front of the queue
                task = std::move(tasks.front());
                tasks.pop();
            } // --- CRITICAL SECTION END (Lock is automatically released!) ---
            
            // Execute the task outside the lock so other threads can still access the queue
            task(); 
        }
    }

public:
    // Constructor: Spin up the threads
    TaskEngine(size_t num_threads) : stop_flag(false) {
        std::cout << "Booting Engine with " << num_threads << " threads...\n";
        
        for (size_t i = 0; i < num_threads; ++i) {
            // Emplace_back constructs the thread in-place.
            // When passing a class member function to a thread, you MUST pass a pointer 
            // to the object instance ('this') as the second argument.
            workers.emplace_back(&TaskEngine::worker_loop, this, i);
        }
    }

    // 5. Destructor: Clean up safely
    ~TaskEngine() {
        std::cout << "\nInitiating Engine Shutdown...\n";
        
        { // Lock the queue to safely flip the stop flag
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }
        
        // BUZZER: Wake up ALL sleeping threads so they can see the stop_flag is true
        condition.notify_all(); 

        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        std::cout << "Engine shutdown complete.\n";
    }

    void submit_void_task(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.push(task);
        }
        // BUZZER: Wake up EXACTLY ONE sleeping thread to process this new task
        condition.notify_one(); 
    }
};

int main() {
    {
        // Scope block to test the constructor and destructor
        TaskEngine engine(4);
        
        // Let's submit 8 simulated heavy calculations to the engine
        for(int i = 1; i <= 8; i++) {
            engine.submit_void_task([i]() {
                std::cout << "Executing task " << i << " on thread " << std::this_thread::get_id() << "\n";
                // Simulate a heavy task taking 500ms
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); 
            });
        }
        
        // Pause the main thread for 3 seconds to let the workers finish cooking
        std::this_thread::sleep_for(std::chrono::seconds(3));

    return 0;
    }
}