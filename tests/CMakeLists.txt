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


# блок 2: тесты рекомендательной системы
add_executable(recommender_tests
    recommender/test_model_mocks.cpp
    recommender/test_decision_logic.cpp
)

target_link_libraries(recommender_tests PRIVATE gtest_main gmock_main)

target_include_directories(recommender_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

gtest_discover_tests(recommender_tests)