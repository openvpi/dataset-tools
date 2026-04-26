cmake_minimum_required(VERSION 3.17)
project(test-find-package LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(dsfw REQUIRED)

add_executable(test-find-package main.cpp)
target_link_libraries(test-find-package PRIVATE dsfw::core dsfw::ui-core)
