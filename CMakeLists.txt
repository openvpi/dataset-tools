cmake_minimum_required(VERSION 3.21)

project(dataset-tools)

# --- Compiler cache (ccache / sccache) ----------------------------------------
find_program(SCCACHE_PROGRAM sccache)
find_program(CCACHE_PROGRAM ccache)
if(SCCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    message(STATUS "Compiler cache: sccache (${SCCACHE_PROGRAM})")
elseif(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Compiler cache: ccache (${CCACHE_PROGRAM})")
    if(MSVC)
        # ccache does not support MSVC /Zi (pdb file lock), use /Z7 instead
        foreach(_config DEBUG RELWITHDEBINFO)
            string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_${_config} "${CMAKE_C_FLAGS_${_config}}")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_${_config} "${CMAKE_CXX_FLAGS_${_config}}")
        endforeach()
    endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (WIN32)
    set(ONNXRUNTIME_ENABLE_DML ON)
endif ()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
endif ()

set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_CMAKE_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/DstoolsHelpers.cmake)

option(DSTOOLS_DEPLOY_MODE "Install only runtime artifacts (exe + DLL + resources), skip SDK files (.lib, headers, cmake config)" OFF)

enable_testing()

add_subdirectory(src)

add_custom_target(layer_audit
    COMMAND ${CMAKE_COMMAND}
        -DAPPS_DIR=${CMAKE_SOURCE_DIR}/src/apps
        -P ${CMAKE_SOURCE_DIR}/cmake/LayerAudit.cmake
    COMMENT "Auditing apps → libs → infer layering rules"
    VERBATIM
)

option(BUILD_EXAMPLES "Build example applications" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )
endif()

# Code quality targets
find_program(CLANG_TIDY_EXE clang-tidy)
find_program(CLANG_FORMAT_EXE clang-format)

if(CLANG_TIDY_EXE AND CLANG_FORMAT_EXE)
    # Target to run clang-tidy on all source files
    add_custom_target(clang-tidy-fix
        COMMAND ${CMAKE_COMMAND}
            -DCLANG_TIDY_EXE=${CLANG_TIDY_EXE}
            -DCLANG_FORMAT_EXE=${CLANG_FORMAT_EXE}
            -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/cmake/RunClangTidy.cmake
        COMMENT "Running clang-tidy with automatic fixes"
        VERBATIM
    )
    
    # Target to format all source files
    add_custom_target(clang-format
        COMMAND ${CMAKE_COMMAND}
            -DCLANG_FORMAT_EXE=${CLANG_FORMAT_EXE}
            -DPROJECT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/cmake/RunClangFormat.cmake
        COMMENT "Formatting all source files with clang-format"
        VERBATIM
    )
    
    # Combined target for code quality
    add_custom_target(code-quality
        COMMAND ${CMAKE_COMMAND} --build . --target clang-tidy-fix
        COMMAND ${CMAKE_COMMAND} --build . --target clang-format
        COMMENT "Running all code quality checks and fixes"
        VERBATIM
    )
    
    message(STATUS "Code quality targets available: clang-tidy-fix, clang-format, code-quality")
else()
    message(WARNING "clang-tidy or clang-format not found. Code quality targets disabled.")
endif()
