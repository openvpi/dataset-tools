cmake_minimum_required(VERSION 3.16)
project(AsyncTaskWindow VERSION 1.0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ----------------------------------
# CMake Settings
# ----------------------------------
if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif ()

if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif ()

if (NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif ()

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

    if (NOT DEFINED CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX "d")
    endif ()
endif ()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
# ----------------------------------
# Main Project
# ----------------------------------
file(GLOB_RECURSE _src include/*.h src/*.h src/*.cpp src/*/*.h src/*/*.cpp)

add_library(${PROJECT_NAME} SHARED)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE ${_src})

target_link_libraries(${PROJECT_NAME} PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE ASYNC_TASK_WINDOW_LIBRARY)

# ----------------------------------
# Add platform specific
# ----------------------------------
if (WIN32)
    set(RC_DESCRIPTION "AsyncTaskWindow.")
    set(RC_COPYRIGHT "Copyright (C) 2026-2026 wolfgitpr")
    include("cmake/winrc.cmake")
endif ()

# ----------------------------------
# link libraries
# ----------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE include src)
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)