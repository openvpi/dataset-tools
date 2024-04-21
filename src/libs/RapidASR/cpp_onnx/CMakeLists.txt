project(FastASR)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(include)
add_subdirectory(src)
#add_subdirectory(tester)
