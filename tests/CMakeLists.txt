# Реализации файловой системы (.cpp), которые нужно слинковать в тесты ядра.
# Подключаем только те, что присутствуют в дереве (Case_A может приходить из
# отдельной ветки), чтобы конфигурация не падала на промежуточных ветках.
set(FS_IMPL_SOURCES "")
foreach(fs_impl IN ITEMS case_A/case_a case_B/case_b case_C/case_c)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${fs_impl}.cpp")
        list(APPEND FS_IMPL_SOURCES "${CMAKE_SOURCE_DIR}/${fs_impl}.cpp")
    endif()
endforeach()

# блок 1: тесты ядра ФС
add_executable(fs_core_tests
    fs_core/test_tree_structure.cpp
    fs_core/test_crud_operations.cpp
    fs_core/test_metrics_tracking.cpp
    fs_core/test_system_state.cpp
    fs_core/test_edge_cases.cpp
    ${FS_IMPL_SOURCES}
)

target_link_libraries(fs_core_tests PRIVATE gtest_main)

# корень проекта главная директорию
target_include_directories(fs_core_tests PRIVATE ${CMAKE_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(fs_core_tests)


# блок 2: тесты рекомендательной системы (Python, recommender/)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME recommender_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest discover
            -s ${CMAKE_SOURCE_DIR}/tests/recommender
            -p "test_*.py"
            -v
)
set_tests_properties(recommender_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# блок 3: тесты бенчмарка
# Линкуем модули бенчмарка напрямую (без main.cpp, в нём своя точка входа).
# Зависимости от case_A/B/C нет: runner тестируется через заглушку IFileSystem.
add_executable(benchmark_tests
    benchmark/test_workload_config.cpp
    benchmark/test_workload_generator.cpp
    benchmark/test_csv_writer.cpp
    benchmark/test_benchmark_runner.cpp
    ${CMAKE_SOURCE_DIR}/benchmark/WorkloadGenerator.cpp
    ${CMAKE_SOURCE_DIR}/benchmark/BenchmarkRunner.cpp
    ${CMAKE_SOURCE_DIR}/benchmark/CsvWriter.cpp
)

target_link_libraries(benchmark_tests PRIVATE gtest_main)

target_include_directories(benchmark_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/benchmark
)

gtest_discover_tests(benchmark_tests)