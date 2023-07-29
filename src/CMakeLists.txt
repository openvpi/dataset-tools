find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Widgets Svg REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Svg REQUIRED)
find_package(QasTool REQUIRED)

add_custom_target(DeployedTargets)

add_subdirectory(apps)
add_subdirectory(libs)

# "QT_QMAKE_EXECUTABLE" is usually defined by QtCreator.
if(NOT DEFINED QT_QMAKE_EXECUTABLE)
    get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
endif()

if(NOT EXISTS "${QT_QMAKE_EXECUTABLE}")
    message(WARNING "deploy_qt_runtime: Can't locate the QMake executable.")
    return()
endif()

cmake_path(GET QT_QMAKE_EXECUTABLE PARENT_PATH __qt_bin_dir)
find_program(QT_DEPLOY_EXECUTABLE NAMES windeployqt macdeployqt HINTS "${__qt_bin_dir}")

if(NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
    message(WARNING "deploy_qt_runtime: Can't locate the deployqt tool.")
    return()
endif()

if(WIN32)
    if(WIN32)
        string(REPLACE "\\" "/" _install_dir ${CMAKE_INSTALL_PREFIX})
    else()
        set(_install_dir ${CMAKE_INSTALL_PREFIX})
    endif()

    install(CODE "
        file(GLOB _files ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.exe)
        foreach(_file \${_files})
            file(COPY \${_file} DESTINATION ${_install_dir})
        endforeach()
    ")

    get_target_property(_targets DeployedTargets TARGETS)

    foreach(_target ${_targets})
        install(CODE "
            set(ENV{PATH} \"${__qt_bin_dir};\$ENV{PATH}\")
            execute_process(
                COMMAND ${QT_DEPLOY_EXECUTABLE}
                --libdir ${_install_dir}
                --plugindir ${_install_dir}/plugins
                --no-translations
                --no-system-d3d-compiler
                --no-compiler-runtime
                --no-opengl-sw
                --force
                --verbose 0
                $<TARGET_FILE:${_target}>
            )
        ")
    endforeach()
elseif(APPLE)
    # To do...
endif()