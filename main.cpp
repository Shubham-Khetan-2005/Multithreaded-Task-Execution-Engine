#include <iostream>
#include <vector>
#include <thread>
#include <chrono> 
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>

struct PriorityTask {
    int priority;
    std::function<void()> task;

    // C++ priority_queue is a Max-Heap. It naturally puts the "largest" element at the top.
    // We must teach it how to compare two PriorityTask tickets.
    bool operator<(const PriorityTask& other) const {
        // A lower number means lower priority.
        // If this ticket has a smaller priority than the 'other' ticket, return true.
        return this->priority < other.priority;
    }
};

class TaskEngine {
private:
    // The container for our worker threads
    std::vector<std::thread> workers;

    // The Queue: Stores simple void functions
    std::priority_queue<PriorityTask> tasks;
    
    // The Synchronization Primitives
    std::mutex queue_mutex;
    std::condition_variable condition;
    
    // A thread-safe boolean to tell workers when to exit their infinite loops
    bool stop_flag;

    // The infinite loop every thread will run
    void worker_loop(int thread_id) {
        std::cout << "Worker " << thread_id << " is online and sleeping...\n";
        
        while (true) {
            std::function<void()> current_task;
            
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

                // Grab the VIP task off the top of the heap
                // priority_queue uses .top() instead of .front()
                current_task = tasks.top().task;
                
                // Remove the ticket from the rack
                tasks.pop();
            } // --- CRITICAL SECTION END (Lock is automatically released!) ---
            
            // Execute the task outside the lock so other threads can still access the queue
            current_task(); 
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

    void submit_task(int priority_level, std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // Wrap the function and its priority into our new data package
            PriorityTask new_ticket;
            new_ticket.priority = priority_level;
            new_ticket.task = std::move(task);
            
            // Push it to the magic rack (it sorts itself automatically!)
            tasks.push(new_ticket);
        }
        
        // Wake up a chef
        condition.notify_one(); 
    }
};

int main() {
    {
        // Scope block to test the constructor and destructor
        TaskEngine engine(4);
        
        // Let's submit 8 simulated heavy calculations to the engine
        for(int i = 1; i <= 5; i++) {
            engine.submit_task(1, [i]() {
                std::cout << "[Priority 1] Executing task " << i << "\n";
                // Simulating a fast task
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
            });
        }
        
        // The manager immediately submits a high-priority VIP Steak (priority 100)
        engine.submit_task(100, []() {
            std::cout << ">>> [Priority 100] Executing VIP task <<<\n";
            // Simulating a heavy task
            std::this_thread::sleep_for(std::chrono::milliseconds(300)); 
        });

        // Wait for everything to cook before destroying the engine
        std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
    }
}