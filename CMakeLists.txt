cmake_minimum_required(VERSION 3.14)

project(InMemoryFileSystem LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# FetchContent нужных либ для тестирования и бенчмарков сюда
# enable_testing()
# add_subdirectory(...) <- сюда директория с тестами

include_directories(${CMAKE_SOURCE_DIR})


# Точка входа проекта здесь
# add_executable(., ...)




