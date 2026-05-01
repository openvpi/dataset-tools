project(audio-util VERSION 1.0.0.0 LANGUAGES CXX)

include(${PROJECT_CMAKE_MODULES_DIR}/infer-target.cmake)

dstools_add_infer_library(${PROJECT_NAME}
    STATIC_OPTION AUDIO_UTIL_BUILD_STATIC
    TESTS_OPTION AUDIO_UTIL_BUILD_TESTS
    INSTALL_OPTION AUDIO_UTIL_INSTALL
    LIBRARY_DEFINE AUDIO_UTIL_LIBRARY
    STATIC_DEFINE AUDIO_UTIL_STATIC
    RC_DESCRIPTION "Audio utils."
    RC_COPYRIGHT "Copyright (C) 2023-2024 wolfgitpr"
)

# Library-specific dependencies
find_package(SndFile CONFIG REQUIRED)
find_package(FLAC CONFIG REQUIRED)
find_package(mpg123 CONFIG REQUIRED)

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PC_Soxr QUIET soxr)
endif()

find_library(Soxr_LIBRARY
    NAMES soxr
    PATHS ${PC_Soxr_LIBRARY_DIRS}
    DOC "Soxr library"
)

target_link_libraries(${PROJECT_NAME} PUBLIC SndFile::sndfile)
target_link_libraries(${PROJECT_NAME} PRIVATE ${Soxr_LIBRARY} MPG123::libmpg123)

# Optional: xsimd for vectorized calculations
find_package(xsimd CONFIG)
if(xsimd_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE xsimd)
    target_compile_definitions(${PROJECT_NAME} PRIVATE AUDIOUTIL_ENABLE_XSIMD)
endif()
