# Deploy logic for Qt runtime and project binaries
# Included from src/CMakeLists.txt
#
# Uses CMake 3.21+ install(RUNTIME_DEPENDENCY_SET) to automatically
# collect and install runtime dependencies (DLLs on Windows, .so on Linux),
# replacing the previous manual file(GLOB *.dll) approach.

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

    # Install runtime dependencies (DLLs) tracked by CMake's dependency graph.
    # This replaces the previous file(GLOB *.dll) approach and automatically
    # resolves all linked shared libraries (vcpkg, project DLLs, etc.).
    install(RUNTIME_DEPENDENCY_SET DstoolsRuntimeDeps
        RUNTIME DESTINATION .
    )

    # Copy ONNX Runtime DLLs (not tracked by CMake target dependencies)
    set(_ORT_LIB_DIR "${PROJECT_DIR}/src/infer/onnxruntime/lib")
    if(IS_DIRECTORY "${_ORT_LIB_DIR}")
        install(CODE "
            file(GLOB _ort_dlls \"${_ORT_LIB_DIR}/*.dll\")
            foreach(_dll \${_ort_dlls})
                file(COPY \${_dll} DESTINATION \"${_install_dir}\")
            endforeach()
        ")
    endif()

    # Run windeployqt for Qt plugins (platforms/, imageformats/, etc.)
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

    # qt.conf — override any absolute-path qt.conf that windeployqt may have
    # generated, ensuring plugins are found via relative paths on end-user machines.
    install(CODE "
        file(WRITE \"${_install_dir}/qt.conf\"
\"[Paths]
Plugins = plugins
\")
    ")
elseif (APPLE)
    find_program(QT_DEPLOY_EXECUTABLE NAMES macdeployqt HINTS "${__qt_bin_dir}")
    if (NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
        message(WARNING "deploy_qt_runtime: Can't locate macdeployqt.")
        return()
    endif ()

    set(_install_dir ${CMAKE_INSTALL_PREFIX})

    get_target_property(_targets DeployedTargets TARGETS)

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

    # Install runtime dependencies (.so files) tracked by CMake's dependency graph.
    # This replaces the previous file(GLOB *.so*) approach.
    install(RUNTIME_DEPENDENCY_SET DstoolsRuntimeDeps
        LIBRARY DESTINATION lib
    )

    # --- Deploy Qt plugins ---
    cmake_path(GET __qt_bin_dir PARENT_PATH __qt_root_dir)
    set(__qt_plugins_dir "${__qt_root_dir}/plugins")

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
    get_target_property(_targets DeployedTargets TARGETS)
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
