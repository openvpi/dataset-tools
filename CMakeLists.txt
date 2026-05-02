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

enable_testing()

add_subdirectory(src)

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
