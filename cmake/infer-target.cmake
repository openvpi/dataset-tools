# cmake/infer-target.cmake
# Common macro for inference library targets.
# Encapsulates: output dirs, MSVC flags, static/shared build, ORT linking,
# install/export boilerplate.
#
# Usage:
#   dstools_add_infer_library(<target>
#       [STATIC_OPTION <option>]
#       [TESTS_OPTION <option>]
#       [INSTALL_OPTION <option>]
#       [LIBRARY_DEFINE <define>]
#       [STATIC_DEFINE <define>]
#       [RC_DESCRIPTION <desc>]
#       [RC_COPYRIGHT <copyright>]
#   )

macro(dstools_add_infer_library _target)
    cmake_parse_arguments(_INF ""
        "STATIC_OPTION;TESTS_OPTION;INSTALL_OPTION;LIBRARY_DEFINE;STATIC_DEFINE;RC_DESCRIPTION;RC_COPYRIGHT"
        "" ${ARGN})

    set(CMAKE_CXX_STANDARD 17)

    # Build options
    if(_INF_STATIC_OPTION)
        option(${_INF_STATIC_OPTION} "Build static library" OFF)
    endif()
    if(_INF_TESTS_OPTION)
        option(${_INF_TESTS_OPTION} "Build test cases" ON)
    endif()
    if(_INF_INSTALL_OPTION)
        option(${_INF_INSTALL_OPTION} "Install library" ON)
    else()
        set(_INF_INSTALL_OPTION "_INF_INSTALL_DUMMY")
        option(${_INF_INSTALL_OPTION} "Install library" ON)
    endif()

    # Output directory defaults
    if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    endif()
    if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    endif()
    if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    endif()

    # MSVC flags
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /manifest:no")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /manifest:no")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /manifest:no")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
        if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
            set(CMAKE_DEBUG_POSTFIX "d")
        endif()
    endif()

    if(${_INF_INSTALL_OPTION})
        include(GNUInstallDirs)
        include(CMakePackageConfigHelpers)
    endif()

    # Gather sources
    include_directories(include)
    file(GLOB_RECURSE _infer_src include/*.h src/*.h src/*.cpp src/*/*.h src/*/*.cpp)

    # Create library
    if(_INF_STATIC_OPTION AND ${_INF_STATIC_OPTION})
        add_library(${_target} STATIC)
        if(_INF_STATIC_DEFINE)
            target_compile_definitions(${_target} PUBLIC ${_INF_STATIC_DEFINE})
        endif()
    else()
        add_library(${_target} SHARED)
    endif()

    if(_INF_LIBRARY_DEFINE)
        target_compile_definitions(${_target} PRIVATE ${_INF_LIBRARY_DEFINE})
    endif()

    target_sources(${_target} PRIVATE ${_infer_src})
    add_library(${_target}::${_target} ALIAS ${_target})

    # ORT path (kept for legacy references in some libs)
    set(ONNX_RUNTIME_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../onnxruntime)

    # Windows RC
    if(WIN32 AND _INF_RC_DESCRIPTION)
        set(RC_DESCRIPTION "${_INF_RC_DESCRIPTION}")
        if(_INF_RC_COPYRIGHT)
            set(RC_COPYRIGHT "${_INF_RC_COPYRIGHT}")
        endif()
        include("cmake/winrc.cmake")
    endif()

    # Include directories
    target_include_directories(${_target} PRIVATE include src)
    target_include_directories(${_target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )

    # Tests
    if(_INF_TESTS_OPTION AND ${_INF_TESTS_OPTION})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
            add_subdirectory(tests)
        endif()
    endif()

    # Install / Export
    if(${_INF_INSTALL_OPTION})
        target_include_directories(${_target} PUBLIC
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

        install(TARGETS ${_target}
            EXPORT ${_target}Targets
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" OPTIONAL
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" OPTIONAL
        )

        install(DIRECTORY include/${_target}
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )

        write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/${_target}ConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY AnyNewerVersion
        )

        configure_package_config_file(
            ${CMAKE_CURRENT_LIST_DIR}/${_target}Config.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/${_target}Config.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${_target}"
            NO_CHECK_REQUIRED_COMPONENTS_MACRO
        )

        install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${_target}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${_target}ConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${_target}"
        )

        install(EXPORT ${_target}Targets
            FILE ${_target}Targets.cmake
            NAMESPACE ${_target}::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${_target}"
        )
    endif()
endmacro()
