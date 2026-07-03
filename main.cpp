#include <iostream>
#include <vector>
#include <thread>
#include <chrono> 
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <future> 
#include <memory> 
#include <type_traits>

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

    template<typename F, typename... Args>
    auto submit(int priority, F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        
        // Figure out what type of answer this function will return (e.g., int, string)
        using return_type = std::invoke_result_t<F, Args...>;

        // Create the Lockbox (packaged_task)
        // std::bind glues the function and its arguments together
        auto lockbox = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        // Grab the buzzer from the lockbox to give to the customer
        std::future<return_type> buzzer = lockbox->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            PriorityTask new_ticket;
            new_ticket.priority = priority;
            
            // Type Erasure: We wrap the complex lockbox inside a simple void() lambda.
            // The magic rack only sees a void function, keeping it happy.
            new_ticket.task = [lockbox]() {
                (*lockbox)(); 
            };
            
            tasks.push(new_ticket);
        }
        
        condition.notify_one(); 
        
        // Return the buzzer immediately, long before the chef finishes cooking
        return buzzer;
    }
};

int main() {
    TaskEngine engine(4);
    
    std::cout << "Submitting a math calculation to the engine...\n";

    // We submit a function that multiplies two numbers and takes some time
    auto math_buzzer = engine.submit(10, [](int a, int b) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return a * b;
    }, 125, 40);

    // Notice how the main thread is NOT frozen! It keeps running.
    std::cout << "Manager is taking other orders while the chef calculates...\n";
    std::cout << "Manager is cleaning the front desk...\n";

    // Now we absolutely need the answer. We press the buzzer.
    // If the chef isn't done, the program pauses right here.
    std::cout << "Waiting for the math result...\n";
    int final_answer = math_buzzer.get();

    std::cout << "The chef returned the answer: " << final_answer << "\n";

    return 0;
}