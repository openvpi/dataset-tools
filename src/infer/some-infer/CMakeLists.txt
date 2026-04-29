cmake_minimum_required(VERSION 3.17)
project(some-infer VERSION 1.0.0.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/infer-target.cmake)

dstools_add_infer_library(${PROJECT_NAME}
    STATIC_OPTION SOME_INFER_BUILD_STATIC
    TESTS_OPTION SOME_INFER_BUILD_TESTS
    INSTALL_OPTION SOME_INFER_INSTALL
    LIBRARY_DEFINE SOME_INFER_LIBRARY
    STATIC_DEFINE SOME_INFER_STATIC
    RC_DESCRIPTION "Some model infer."
    RC_COPYRIGHT "Copyright (C) 2023-2024 wolfgitpr"
)

# Library-specific dependencies
find_package(SndFile CONFIG REQUIRED)
find_package(wolf-midi CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC SndFile::sndfile audio-util wolf-midi::wolf-midi dstools-infer-common)
target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime)
