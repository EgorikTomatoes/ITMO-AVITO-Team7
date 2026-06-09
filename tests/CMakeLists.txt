# блок 1: тесты ядра ФС
add_executable(fs_core_tests
    fs_core/test_crud_operations.cpp
    fs_core/test_metrics_tracking.cpp
    fs_core/test_edge_cases.cpp
)

target_link_libraries(fs_core_tests PRIVATE gtest_main)

# корень проекта главная директорию
target_include_directories(fs_core_tests PRIVATE ${CMAKE_SOURCE_DIR})

include(GoogleTest)
gtest_discover_tests(fs_core_tests)


# блок 2: тесты рекомендательной системы
add_executable(recommender_tests
    recommender/test_model_mocks.cpp
    recommender/test_decision_logic.cpp
)

target_link_libraries(recommender_tests PRIVATE gtest_main gmock_main)

target_include_directories(recommender_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

gtest_discover_tests(recommender_tests)