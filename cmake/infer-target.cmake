# cmake/infer-target.cmake
# Common function for inference library targets.
# Wraps dstools_add_library with inference-specific boilerplate:
# shared/static toggle, ORT linking, install/export, winrc, tests.
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

function(dstools_add_infer_library _target)
    cmake_parse_arguments(_INF ""
        "STATIC_OPTION;TESTS_OPTION;INSTALL_OPTION;LIBRARY_DEFINE;STATIC_DEFINE;RC_DESCRIPTION;RC_COPYRIGHT"
        "" ${ARGN})

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

    # Determine library type
    set(_lib_type SHARED)
    if(DEFINED _INF_STATIC_OPTION)
        if(${_INF_STATIC_OPTION})
            set(_lib_type STATIC)
        endif()
    endif()

    # Compute export set name (only when installing)
    if(${_INF_INSTALL_OPTION})
        set(_export_arg EXPORT_SET ${_target}Targets)
    else()
        set(_export_arg "")
    endif()

    # Create library via dstools_add_library
    dstools_add_library(${_target}
        ${_lib_type}
        CXX_STANDARD 17
        NAMESPACE ${_target}::${_target}
        ${_export_arg}
    )

    # Static define
    if(_lib_type STREQUAL "STATIC" AND _INF_STATIC_DEFINE)
        target_compile_definitions(${_target} PUBLIC ${_INF_STATIC_DEFINE})
    endif()

    # Library export define
    if(_INF_LIBRARY_DEFINE)
        target_compile_definitions(${_target} PRIVATE ${_INF_LIBRARY_DEFINE})
    endif()

    # ORT path (kept for legacy references in some libs)
    set(ONNX_RUNTIME_PATH ${PROJECT_DIR}/src/infer/onnxruntime PARENT_SCOPE)

    # Windows RC
    if(WIN32 AND _INF_RC_DESCRIPTION)
        set(RC_DESCRIPTION "${_INF_RC_DESCRIPTION}")
        if(_INF_RC_COPYRIGHT)
            set(RC_COPYRIGHT "${_INF_RC_COPYRIGHT}")
        endif()
        include("cmake/winrc.cmake")
    endif()

    # Tests
    if(DEFINED _INF_TESTS_OPTION)
        if(${_INF_TESTS_OPTION})
            if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
                set(CMAKE_FOLDER "Tests")
                add_subdirectory(tests)
                set(CMAKE_FOLDER "Libraries/Infer")
            endif()
        endif()
    endif()

    # Install / Export (beyond what dstools_add_library already does)
    if(${_INF_INSTALL_OPTION})
        include(GNUInstallDirs)
        include(CMakePackageConfigHelpers)

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
endfunction()
