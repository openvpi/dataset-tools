# https://github.com/CrSjimo/talcs/blob/main/lib/CMakeLists.txt
project(r8brain VERSION 6.5 LANGUAGES CXX C)

file(GLOB_RECURSE _src *.h *.cpp *.c)

list(FILTER _src EXCLUDE REGEX bench)
list(FILTER _src EXCLUDE REGEX DLL)
list(FILTER _src EXCLUDE REGEX other)
list(FILTER _src EXCLUDE REGEX example.cpp)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_library(r8brain STATIC ${_src})

target_include_directories(r8brain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/r8brain-free-src)
