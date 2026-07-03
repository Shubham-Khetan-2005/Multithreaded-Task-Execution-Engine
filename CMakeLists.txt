# Require a modern version of CMake
cmake_minimum_required(VERSION 3.10)

# Define the project name, version, and primary language
project(MultithreadedTaskEngine VERSION 1.0.0 LANGUAGES CXX)

# Strictly enforce C++17 to support std::invoke_result_t and variadic templates
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable maximum performance optimizations for release builds
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Locate the system's native threading library (handles -pthread automatically)
find_package(Threads REQUIRED)

# Expose the include directory so targets can find TaskEngine.hpp
include_directories(include)

# ---------------------------------------------------------
# Build Targets
# ---------------------------------------------------------

# Target 1: The Telemetry Benchmark
add_executable(benchmark tests/benchmark.cpp)
target_link_libraries(benchmark PRIVATE Threads::Threads)

# Target 2: The Basic Usage Example
add_executable(basic_usage src/examples/basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE Threads::Threads)

# Target 3: The Concurrent Web Scraper Example
add_executable(web_scraper src/examples/web_scraper.cpp)
target_link_libraries(web_scraper PRIVATE Threads::Threads)