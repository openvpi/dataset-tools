option(BUILD_TESTS "Build unit test targets" ON)

# Infer tests require ONNX models and audio files — off by default
if (NOT BUILD_TESTS)
    set(GAME_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SOME_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(RMVPE_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(AUDIO_UTIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
else()
    # Default infer tests to OFF even when BUILD_TESTS=ON
    # Enable individually: -DGAME_INFER_BUILD_TESTS=ON etc.
    if(NOT DEFINED CACHE{GAME_INFER_BUILD_TESTS})
        set(GAME_INFER_BUILD_TESTS OFF CACHE BOOL "Build TestGame (requires GAME model)" FORCE)
    endif()
    if(NOT DEFINED CACHE{SOME_INFER_BUILD_TESTS})
        set(SOME_INFER_BUILD_TESTS OFF CACHE BOOL "Build TestSome (requires SOME model)" FORCE)
    endif()
    if(NOT DEFINED CACHE{RMVPE_INFER_BUILD_TESTS})
        set(RMVPE_INFER_BUILD_TESTS OFF CACHE BOOL "Build TestRmvpe (requires RMVPE model)" FORCE)
    endif()
    if(NOT DEFINED CACHE{AUDIO_UTIL_BUILD_TESTS})
        set(AUDIO_UTIL_BUILD_TESTS OFF CACHE BOOL "Build TestAudioUtil" FORCE)
    endif()
endif ()

find_package(QT NAMES Qt6 COMPONENTS Core Gui Widgets Svg Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets Svg Network REQUIRED)
qt_standard_project_setup()

add_custom_target(DeployedTargets)

# Shared types (header-only, must come before everything)
set(CMAKE_FOLDER "Libraries/Framework")
add_subdirectory(types)

# Framework layers
add_subdirectory(framework)

# Shared layers (must come before apps)
set(CMAKE_FOLDER "Libraries")
add_subdirectory(domain)
add_subdirectory(ui-core)
add_subdirectory(widgets)

set(CMAKE_FOLDER "Libraries/Infer")
add_subdirectory(infer)

set(CMAKE_FOLDER "Libraries/Libs")
add_subdirectory(libs)

# Applications
unset(CMAKE_FOLDER)
add_subdirectory(apps)

if (BUILD_TESTS)
    set(CMAKE_FOLDER "Tests")
    add_subdirectory(tests)
endif ()

include(${CMAKE_SOURCE_DIR}/cmake/deploy.cmake)