cmake_minimum_required(VERSION 3.16)

project(InMemoryFileSystem LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Interface
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/case_A
    ${CMAKE_CURRENT_SOURCE_DIR}/case_B
    ${CMAKE_CURRENT_SOURCE_DIR}/case_C
    ${CMAKE_CURRENT_SOURCE_DIR}/benchmark
)

add_library(filesystem_cases
    case_A/case_a.cpp
    case_B/case_b.cpp
    case_C/case_c.cpp
    

    Interface/file_system.hpp

    case_A/case_a.hpp
    case_B/case_b.hpp
    case_C/case_c.hpp

    utils/memory_pool.hpp
    utils/path_utils.hpp
)

target_include_directories(filesystem_cases PUBLIC
    ${PROJECT_INCLUDE_DIRS}
)

target_compile_features(filesystem_cases PUBLIC cxx_std_23)

add_library(benchmark_core
    benchmark/BenchmarkRunner.cpp
    benchmark/CsvWriter.cpp
    benchmark/WorkloadGenerator.cpp

    benchmark/BenchmarkRunner.hpp
    benchmark/CsvWriter.hpp
    benchmark/WorkloadGenerator.hpp
    benchmark/WorkloadConfig.hpp
    benchmark/Operations.hpp
)

target_include_directories(benchmark_core PUBLIC
    ${PROJECT_INCLUDE_DIRS}
)

target_link_libraries(benchmark_core PUBLIC
    filesystem_cases
)

target_compile_features(benchmark_core PUBLIC cxx_std_23)

add_executable(benchmark_app
    benchmark/main.cpp
)

target_link_libraries(benchmark_app PRIVATE
    benchmark_core
)

target_compile_features(benchmark_app PRIVATE cxx_std_23)

if (MSVC)
    target_compile_options(filesystem_cases PRIVATE /W4 /utf-8)
    target_compile_options(benchmark_core PRIVATE /W4 /utf-8)
    target_compile_options(benchmark_app PRIVATE /W4 /utf-8)
else()
    target_compile_options(filesystem_cases PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(benchmark_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(benchmark_app PRIVATE -Wall -Wextra -Wpedantic)
endif()