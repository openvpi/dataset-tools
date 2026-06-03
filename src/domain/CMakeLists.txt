project(dstools-domain VERSION 3.0.0)

set(DSTOOLS_DOMAIN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(DSTOOLS_DOMAIN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(DSTOOLS_DOMAIN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(DSTOOLS_DOMAIN_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

configure_file(
        include/dstools/Version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/dstools/Version.h
        @ONLY
)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        DEPENDS
        PUBLIC dsfw-core dstools-types
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network nlohmann_json::nlohmann_json
        PRIVATE textgrid SndFile::sndfile audio-util
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)
