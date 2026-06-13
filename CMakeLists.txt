cmake_minimum_required(VERSION 3.20)

project(course_allocation_system
    VERSION 1.0.0
    DESCRIPTION "Capacity-aware course allocation using modified Gale-Shapley matching"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(COURSE_ALLOCATION_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(course_allocation
    src/allocator.cpp
    src/csv.cpp
)

target_include_directories(course_allocation PUBLIC include)
target_compile_options(course_allocation PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(COURSE_ALLOCATION_ENABLE_SANITIZERS)
    target_compile_options(course_allocation PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(course_allocation PUBLIC -fsanitize=address,undefined)
endif()

add_executable(course_allocator src/main.cpp)
target_link_libraries(course_allocator PRIVATE course_allocation)
target_compile_options(course_allocator PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(course_allocator_benchmark src/benchmark.cpp)
target_link_libraries(course_allocator_benchmark PRIVATE course_allocation)
target_compile_options(course_allocator_benchmark PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

enable_testing()

add_executable(course_allocation_tests tests/test_allocator.cpp)
target_link_libraries(course_allocation_tests PRIVATE course_allocation)
target_compile_options(course_allocation_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_test(NAME course_allocation_tests COMMAND course_allocation_tests)

