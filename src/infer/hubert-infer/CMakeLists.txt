# ----------------------------------
# Project
# ----------------------------------
cmake_minimum_required(VERSION 3.17)
project(hubert-infer VERSION 1.0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# ----------------------------------
# Build Options
# ----------------------------------
option(HUBERT_INFER_BUILD_STATIC "Build static library" OFF)
option(HUBERT_INFER_INSTALL "Install library" ON)

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

if (HUBERT_INFER_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
endif ()

# ----------------------------------
# Main Project
# ----------------------------------
include_directories(include)
file(GLOB_RECURSE _src include/*.h src/*.h src/*.cpp)

if (HUBERT_INFER_BUILD_STATIC)
    add_library(${PROJECT_NAME} STATIC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC HUBERT_INFER_STATIC)
else ()
    add_library(${PROJECT_NAME} SHARED)
endif ()
target_compile_definitions(${PROJECT_NAME} PRIVATE HUBERT_INFER_LIBRARY)

find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json REQUIRED)

set(ONNX_RUNTIME_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../onnxruntime)

target_link_libraries(${PROJECT_NAME} PUBLIC onnxruntime dstools-infer-common)
target_link_libraries(${PROJECT_NAME} PRIVATE SndFile::sndfile nlohmann_json::nlohmann_json audio-util dstools-core)

target_sources(${PROJECT_NAME} PRIVATE ${_src})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# ----------------------------------
# Add platform specific
# ----------------------------------
if (WIN32)
    set(RC_DESCRIPTION "HuBERT forced alignment infer.")
    set(RC_COPYRIGHT "Copyright (C) 2024-2026")
    include("cmake/winrc.cmake")
endif ()

# ----------------------------------
# link libraries
# ----------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE include src)
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# ----------------------------------
# install
# ----------------------------------
if (HUBERT_INFER_INSTALL)
    target_include_directories(${PROJECT_NAME} PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    install(TARGETS ${PROJECT_NAME}
            EXPORT ${PROJECT_NAME}Targets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
    )

    install(DIRECTORY include/${PROJECT_NAME}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
    )

    configure_package_config_file(
            ${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Config.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
            NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )
endif ()
