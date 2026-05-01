project(dsfw-widgets)

set(CMAKE_AUTOMOC ON)

set(DSFW_WIDGETS_SOURCES
    src/PlayWidget.cpp
    src/TaskWindow.cpp
    src/ShortcutManager.cpp
    src/ShortcutEditorWidget.cpp
    src/PathSelector.cpp
    src/ViewportController.cpp
    src/RunProgressRow.cpp
    src/FileProgressTracker.cpp
    src/PropertyEditor.cpp
    src/SettingsDialog.cpp
    src/LogViewer.cpp
    src/ProgressDialog.cpp
)

set(DSFW_WIDGETS_HEADERS
    include/dsfw/widgets/WidgetsGlobal.h
    include/dsfw/widgets/PlayWidget.h
    include/dsfw/widgets/TaskWindow.h
    include/dsfw/widgets/ShortcutManager.h
    include/dsfw/widgets/ShortcutEditorWidget.h
    include/dsfw/widgets/PathSelector.h
    include/dsfw/widgets/ViewportController.h
    include/dsfw/widgets/RunProgressRow.h
    include/dsfw/widgets/FileProgressTracker.h
    include/dsfw/widgets/PropertyEditor.h
    include/dsfw/widgets/SettingsDialog.h
    include/dsfw/widgets/LogViewer.h
    include/dsfw/widgets/ProgressDialog.h
)

add_library(${PROJECT_NAME} SHARED ${DSFW_WIDGETS_SOURCES} ${DSFW_WIDGETS_HEADERS})
add_library(dsfw::widgets ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-ui-core
    dsfw-core
    dstools-audio
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE DSFW_WIDGETS_BUILD)

# Install rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT dsfwTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
