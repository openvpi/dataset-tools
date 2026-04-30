cmake_minimum_required(VERSION 3.17)
project(game-infer VERSION 1.0.0.0 LANGUAGES CXX)

include(${PROJECT_CMAKE_MODULES_DIR}/infer-target.cmake)

dstools_add_infer_library(${PROJECT_NAME}
        STATIC_OPTION GAME_INFER_BUILD_STATIC
        TESTS_OPTION GAME_INFER_BUILD_TESTS
        INSTALL_OPTION GAME_INFER_INSTALL
        LIBRARY_DEFINE GAME_INFER_LIBRARY
        STATIC_DEFINE GAME_INFER_STATIC
        RC_DESCRIPTION "Game model infer."
        RC_COPYRIGHT "Copyright (C) 2026-2026 wolfgitpr"
)

# Library-specific dependencies
find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(wolf-midi CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC SndFile::sndfile nlohmann_json::nlohmann_json audio-util wolf-midi::wolf-midi dstools-infer-common)
target_link_libraries(${PROJECT_NAME} PRIVATE dstools-core)