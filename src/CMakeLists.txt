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

# "QT_QMAKE_EXECUTABLE" is usually defined by QtCreator.
if (NOT DEFINED QT_QMAKE_EXECUTABLE)
    get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
endif ()

if (NOT EXISTS "${QT_QMAKE_EXECUTABLE}")
    message(WARNING "deploy_qt_runtime: Can't locate the QMake executable.")
    return()
endif ()

cmake_path(GET QT_QMAKE_EXECUTABLE PARENT_PATH __qt_bin_dir)

if (NOT CMAKE_INSTALL_PREFIX)
    return()
endif ()

if (WIN32)
    find_program(QT_DEPLOY_EXECUTABLE NAMES windeployqt HINTS "${__qt_bin_dir}")
    if (NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
        message(WARNING "deploy_qt_runtime: Can't locate windeployqt.")
        return()
    endif ()

    string(REPLACE "\\" "/" _install_dir ${CMAKE_INSTALL_PREFIX})

    get_target_property(_deploy_targets DeployedTargets TARGETS)
    # Copy only deployed executables (not test executables)
    foreach (_target ${_deploy_targets})
        install(CODE "
            file(COPY \"$<TARGET_FILE:${_target}>\" DESTINATION \"${_install_dir}\")
        ")
    endforeach ()
    # Copy all DLLs (shared libraries needed at runtime)
    install(CODE "
        file(GLOB _dlls \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll\")
        foreach(_dll \${_dlls})
            file(COPY \${_dll} DESTINATION \"${_install_dir}\")
        endforeach()
    ")

    get_target_property(_targets DeployedTargets TARGETS)

    foreach (_target ${_targets})
        install(CODE "
            set(ENV{PATH} \"${__qt_bin_dir};\$ENV{PATH}\")
            execute_process(
                COMMAND \"${QT_DEPLOY_EXECUTABLE}\"
                --libdir \"${_install_dir}\"
                --plugindir \"${_install_dir}/plugins\"
                --no-translations
                --no-system-d3d-compiler
                --no-compiler-runtime
                --no-opengl-sw
                --force
                --verbose 0
                \"${_install_dir}/$<TARGET_FILE_NAME:${_target}>\"
                WORKING_DIRECTORY \"${_install_dir}\"
            )
        ")
    endforeach ()
elseif (APPLE)
    find_program(QT_DEPLOY_EXECUTABLE NAMES macdeployqt HINTS "${__qt_bin_dir}")
    if (NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
        message(WARNING "deploy_qt_runtime: Can't locate macdeployqt.")
        return()
    endif ()

    set(_install_dir ${CMAKE_INSTALL_PREFIX})

    get_target_property(_targets DeployedTargets TARGETS)

    foreach (_target ${_targets})
        install(CODE "
            file(COPY \"$<TARGET_BUNDLE_DIR:${_target}>\" DESTINATION \"${_install_dir}\")
        ")
    endforeach ()

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        foreach (_target ${_targets})
            install(CODE "
                execute_process(
                    COMMAND \"${QT_DEPLOY_EXECUTABLE}\"
                    \"${_install_dir}/$<TARGET_FILE_NAME:${_target}>.app\"
                    -verbose=0
                    -codesign=-
                    WORKING_DIRECTORY \"${_install_dir}\"
                )
            ")
        endforeach ()
    else ()
        foreach (_target ${_targets})
            install(CODE "
                execute_process(
                    COMMAND \"${QT_DEPLOY_EXECUTABLE}\"
                    \"${_install_dir}/$<TARGET_FILE_NAME:${_target}>.app\"
                    -verbose=0
                    WORKING_DIRECTORY \"${_install_dir}\"
                )
            ")
        endforeach ()
    endif ()
elseif (UNIX)
    set(_install_dir ${CMAKE_INSTALL_PREFIX})

    # --- Copy project shared libraries (.so) ---
    install(CODE "
        file(GLOB _files \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.so*\")
        foreach(_file \${_files})
            file(COPY \${_file} DESTINATION \"${_install_dir}/lib\")
        endforeach()
    ")
    install(CODE "
        file(GLOB _lib_files \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.so*\")
        foreach(_file \${_lib_files})
            file(COPY \${_file} DESTINATION \"${_install_dir}/lib\")
        endforeach()
    ")

    # --- Copy deployed executables ---
    get_target_property(_targets DeployedTargets TARGETS)

    foreach (_target ${_targets})
        install(CODE "
            file(COPY \"$<TARGET_FILE:${_target}>\" DESTINATION \"${_install_dir}/bin\")
        ")
    endforeach ()

    # --- Deploy Qt shared libraries and plugins ---
    cmake_path(GET __qt_bin_dir PARENT_PATH __qt_root_dir)
    set(__qt_lib_dir "${__qt_root_dir}/lib")
    set(__qt_plugins_dir "${__qt_root_dir}/plugins")

    # Qt shared libraries
    install(CODE "
        set(_qt_lib_dir \"${__qt_lib_dir}\")
        set(_install_dir \"${_install_dir}\")
        foreach(_lib Core Gui Widgets Svg Network DBus OpenGL XcbQpa WaylandClient WaylandEglClientHwIntegration)
            file(GLOB _matches \"\${_qt_lib_dir}/libQt6\${_lib}.so*\")
            foreach(_f \${_matches})
                file(COPY \${_f} DESTINATION \"\${_install_dir}/lib\")
            endforeach()
        endforeach()

        # Copy ICU libraries if present (Qt dependency)
        file(GLOB _icu_libs \"\${_qt_lib_dir}/libicu*.so*\")
        foreach(_f \${_icu_libs})
            file(COPY \${_f} DESTINATION \"\${_install_dir}/lib\")
        endforeach()
    ")

    # Qt plugins
    install(CODE "
        set(_qt_plugins_dir \"${__qt_plugins_dir}\")
        set(_install_dir \"${_install_dir}\")
        foreach(_plugin_dir platforms platforminputcontexts platformthemes imageformats iconengines xcbglintegrations wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration networkinformation tls)
            if(IS_DIRECTORY \"\${_qt_plugins_dir}/\${_plugin_dir}\")
                file(COPY \"\${_qt_plugins_dir}/\${_plugin_dir}\" DESTINATION \"\${_install_dir}/plugins\")
            endif()
        endforeach()
    ")

    # qt.conf — tells Qt where to find plugins and libraries relative to the binary
    install(CODE "
        set(_install_dir \"${_install_dir}\")
        file(WRITE \"\${_install_dir}/bin/qt.conf\"
\"[Paths]
Prefix = ..
Libraries = lib
Plugins = plugins
\")
    ")

    # Generate launcher scripts that set LD_LIBRARY_PATH
    foreach (_target ${_targets})
        install(CODE "
            set(_install_dir \"${_install_dir}\")
            set(_name \"$<TARGET_FILE_NAME:${_target}>\")
            file(WRITE \"\${_install_dir}/\${_name}.sh\"
\"#!/bin/bash
DIR=\\\"\\$( cd \\\"\\$( dirname \\\"\\$\\{BASH_SOURCE[0]\\}\\\" )\\\" && pwd )\\\"
export LD_LIBRARY_PATH=\\\"\\\$DIR/lib:\\\$LD_LIBRARY_PATH\\\"
exec \\\"\\\$DIR/bin/\${_name}\\\" \\\"\\$@\\\"
\")
            execute_process(COMMAND chmod +x \"\${_install_dir}/\${_name}.sh\")
        ")
    endforeach ()
endif ()