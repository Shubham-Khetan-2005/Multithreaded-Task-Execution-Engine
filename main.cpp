#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono> // Used just to simulate idling for today

class TaskEngine {
private:
    // 1. The container for our worker threads
    std::vector<std::thread> workers;
    
    // 2. A thread-safe boolean to tell workers when to exit their infinite loops
    std::atomic<bool> stop_flag;

    // 3. The infinite loop every thread will run
    void worker_loop(int thread_id) {
        std::cout << "Worker " << thread_id << " is online and waiting...\n";
        
        while (!stop_flag) {
            // WARNING: This is a "busy wait". It wastes CPU cycles.
            // We will fix this in Day 2 using std::condition_variable.
            // For today, we use a small sleep so it doesn't melt your processor.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        std::cout << "Worker " << thread_id << " is shutting down.\n";
    }

public:
    // 4. Constructor: Spin up the threads
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
        
        // Flip the switch. All threads will exit their while loops.
        stop_flag = true;
        
        // Wait for every thread to finish its current loop iteration and terminate.
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join(); // Blocks the main thread until 'worker' finishes
            }
        }
        
        std::cout << "All workers joined. Engine shutdown complete.\n";
    }
};

int main() {
    {
        // Scope block to test the constructor and destructor
        TaskEngine engine(4);
        
        // Let the main thread sleep for a second to watch the workers run
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
    } // The engine object goes out of scope here, triggering the destructor automatically!

    return 0;
}