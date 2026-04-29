cmake_minimum_required(VERSION 3.17)
project(hubert-infer VERSION 1.0.0.0 LANGUAGES CXX)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/infer-target.cmake)

dstools_add_infer_library(${PROJECT_NAME}
    STATIC_OPTION HUBERT_INFER_BUILD_STATIC
    INSTALL_OPTION HUBERT_INFER_INSTALL
    LIBRARY_DEFINE HUBERT_INFER_LIBRARY
    STATIC_DEFINE HUBERT_INFER_STATIC
    RC_DESCRIPTION "HuBERT forced alignment infer."
    RC_COPYRIGHT "Copyright (C) 2024-2026"
)

# Library-specific dependencies
find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC onnxruntime dstools-infer-common)
target_link_libraries(${PROJECT_NAME} PRIVATE SndFile::sndfile nlohmann_json::nlohmann_json audio-util dstools-core)
