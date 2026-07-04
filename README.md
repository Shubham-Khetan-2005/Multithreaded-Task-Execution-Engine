# Multithreaded Priority Task Engine

A lightweight, header-only **C++17 task execution engine** built around a fixed worker thread pool and priority-based scheduling.

The engine allows arbitrary callable tasks to be submitted with a priority, executes queued work concurrently across reusable worker threads, and exposes asynchronous results through `std::future`.

## Features

* Fixed-size **worker thread pool**
* **Priority-based task scheduling**
* Generic task submission using **variadic templates**
* Automatic return-type deduction
* Asynchronous result retrieval with `std::future`
* Exception propagation from worker tasks to callers
* Efficient worker sleeping using `std::condition_variable`
* Graceful shutdown with queued-task draining
* Thread-safe concurrent task submission
* Header-only C++17 implementation

## Architecture

```text
                    submit(priority, task, args...)
                                  |
                                  v
                         +------------------+
                         |  Priority Queue  |
                         |                  |
                         |  High Priority   |
                         |        |         |
                         |        v         |
                         |  Low Priority    |
                         +--------+---------+
                                  |
                    +-------------+-------------+
                    |             |             |
                    v             v             v
                Worker 1      Worker 2       Worker N
                    |             |             |
                    +-------------+-------------+
                                  |
                                  v
                       Shared Async State
                                  |
                                  v
                           std::future<T>
```

Submitted tasks are wrapped into a common zero-argument callable representation and inserted into a shared priority queue.

Worker threads sleep while no work is available. When a task is submitted, an idle worker is notified, removes the highest-priority queued task, releases the queue lock, and executes the task independently.

The engine uses **non-preemptive priority scheduling**: higher-priority queued tasks are selected first, but a task that is already executing is never interrupted.

## Project Structure

```text
multithreaded-task-engine/
├── include/
│   └── TaskEngine.hpp
├── src/
│   └── examples/
│       ├── basic_usage.cpp
│       └── web_scraper.cpp
├── tests/
│   ├── benchmark.cpp
│   └── unit_tests.cpp
├── CMakeLists.txt
├── .gitignore
└── README.md
```

| File                           | Purpose                                |
| ------------------------------ | -------------------------------------- |
| `include/TaskEngine.hpp`       | Header-only task engine implementation |
| `src/examples/basic_usage.cpp` | Basic priority and future usage        |
| `src/examples/web_scraper.cpp` | Simulated concurrent I/O workload      |
| `tests/unit_tests.cpp`         | Correctness and scheduling tests       |
| `tests/benchmark.cpp`          | 100,000-task stress benchmark          |

## Quick Start

### Requirements

* C++17 compatible compiler
* CMake 3.10+
* POSIX or platform-supported C++ threading implementation

### Build

```bash
git clone https://github.com/Shubham-Khetan-2005/Multithreaded-Task-Execution-Engine.git
cd multithreaded-task-engine

# Generate build files
mkdir build && cd build
cmake ..

# Compile the executables
make

# Run the test suite and benchmarks
./basic_usage
./unit_tests
./benchmark
./web_scraper
```

## Usage

```cpp
#include "TaskEngine.hpp"

#include <iostream>

int main() {
    TaskEngine engine(4);

    auto result = engine.submit(
        10,
        [](int a, int b) {
            return a + b;
        },
        20,
        30
    );

    std::cout << result.get() << '\n';

    return 0;
}
```

Output:

```text
50
```

`submit()` accepts:

```cpp
engine.submit(priority, callable, arguments...);
```

and automatically returns:

```cpp
std::future<ReturnType>
```

where `ReturnType` is deduced from the submitted callable and its arguments.

## Priority Scheduling

Every submitted task is associated with an integer priority.

```cpp
auto low_priority = engine.submit(1, []() {
    // Background work
});

auto high_priority = engine.submit(10, []() {
    // More urgent work
});
```

When multiple tasks are waiting in the queue, the task with the highest priority is selected first.

```text
Queued Tasks

Priority 10  <--- selected first
Priority 5
Priority 2
Priority 1
```

Priority scheduling is **non-preemptive**.

If a low-priority task is already running when a higher-priority task arrives, the running task continues to completion. The higher-priority task is preferred the next time a worker selects work from the queue.

Tasks with equal priority do not currently have guaranteed FIFO ordering.

## Generic Task Submission

The engine can execute functions, lambdas, and other compatible callables with arbitrary arguments and return types.

### Returning a Value

```cpp
auto future = engine.submit(
    5,
    [](int a, int b) {
        return a * b;
    },
    21,
    2
);

std::cout << future.get();
```

Output:

```text
42
```

### Returning a String

```cpp
auto future = engine.submit(
    5,
    []() -> std::string {
        return "Task completed";
    }
);

std::cout << future.get();
```

### Fire-and-Forget Work

```cpp
engine.submit(1, []() {
    perform_background_work();
});
```

The returned `std::future<void>` may be ignored when the caller does not need to wait for or retrieve a result.

## Benchmark

The benchmark submits **100,000 tasks** to an engine running with **10 worker threads**.

Measured execution times:

|         Run | Execution Time |
| ----------: | -------------: |
|           1 |     396.872 ms |
|           2 |     375.260 ms |
|           3 |     388.528 ms |
|           4 |     397.626 ms |
|           5 |     391.625 ms |
|           6 |     399.415 ms |
| **Average** | **391.554 ms** |

Average benchmark throughput:

```text
100,000 tasks / 0.391554 seconds
≈ 255,000 tasks/second
```

The benchmark measures the complete engine lifecycle:

```text
Engine creation
      +
100,000 task submissions
      +
Task execution
      +
Future synchronization
      +
Worker shutdown and joining
```

### Sample Benchmark Output

```text
Starting benchmark with 100000 tasks...
Booting engine with 10 threads.
Successfully processed 100000 tasks.
Total execution time: 391.625 ms.
```

> Benchmark results are workload- and hardware-dependent. The reported throughput reflects the included benchmark workload and should not be interpreted as a universal task execution rate.

## Complexity

Let `Q` be the number of tasks currently waiting in the priority queue.

| Operation                    | Complexity                 |
| ---------------------------- | -------------------------- |
| Submit task                  | `O(log Q)`                 |
| Select highest-priority task | `O(1)`                     |
| Remove selected task         | `O(log Q)`                 |
| Worker idle waiting          | Blocking / no busy polling |

Task execution complexity depends entirely on the submitted callable.

## Testing

Run the unit tests from the build directory:

```bash
./unit_tests
```

The test suite validates:

* future return-value propagation
* priority-based queued task selection
* invalid zero-worker construction

Run the stress benchmark with:

```bash
./benchmark
```

The benchmark submits 100,000 tasks and verifies that every task completes successfully.

## Technologies and Concepts

* C++17
* Multithreading
* Thread pools
* Priority scheduling
* Producer-consumer synchronization
* Mutexes
* Condition variables
* RAII
* Variadic templates
* Forwarding references
* Perfect forwarding
* Type erasure
* `std::packaged_task`
* `std::future`
* Asynchronous shared state
* CMake

## License

This project is available for educational and experimental use.
