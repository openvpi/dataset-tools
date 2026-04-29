option(BUILD_TESTS "Build all test targets" ON)

if (NOT BUILD_TESTS)
    set(GAME_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SOME_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(RMVPE_INFER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(AUDIO_UTIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
endif()

find_package(QT NAMES Qt6 COMPONENTS Core Widgets Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Svg REQUIRED)

add_custom_target(DeployedTargets)

# Shared layers (must come before apps)
add_subdirectory(core)
add_subdirectory(audio)
add_subdirectory(widgets)
add_subdirectory(libs)
add_subdirectory(infer)

add_subdirectory(apps)

if (BUILD_TESTS)
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
find_program(QT_DEPLOY_EXECUTABLE NAMES windeployqt macdeployqt HINTS "${__qt_bin_dir}")

if (NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
    message(WARNING "deploy_qt_runtime: Can't locate the deployqt tool.")
    return()
endif ()

if (NOT CMAKE_INSTALL_PREFIX)
    return()
endif ()

if (WIN32)
    string(REPLACE "\\" "/" _install_dir ${CMAKE_INSTALL_PREFIX})

    get_target_property(_deploy_targets DeployedTargets TARGETS)
    # Copy only deployed executables (not test executables)
    foreach(_target ${_deploy_targets})
        install(CODE "
            file(COPY \"$<TARGET_FILE:${_target}>\" DESTINATION \"${_install_dir}\")
        ")
    endforeach()
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

    install(CODE "
        file(GLOB _files \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.so*\" \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.exe\")
        foreach(_file \${_files})
            file(COPY \${_file} DESTINATION \"${_install_dir}\")
        endforeach()
    ")

    get_target_property(_targets DeployedTargets TARGETS)

    foreach (_target ${_targets})
        install(CODE "
            file(COPY \"$<TARGET_FILE:${_target}>\" DESTINATION \"${_install_dir}\")
        ")
    endforeach ()
endif ()