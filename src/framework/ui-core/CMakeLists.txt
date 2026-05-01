project(dsfw-ui-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Standalone build guards — only activate when targets are not already defined
if(NOT TARGET dsfw::core)
    find_package(dsfw CONFIG REQUIRED)
endif()

if(NOT TARGET Qt6::Core)
    find_package(QT NAMES Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)
endif()

set(DSFW_UI_CORE_SOURCES
    src/Theme.cpp
    src/FramelessHelper.cpp
    src/IconNavBar.cpp
    src/AppShell.cpp
    src/PluginManager.cpp
)

set(DSFW_UI_CORE_HEADERS
    include/dsfw/Theme.h
    include/dsfw/FramelessHelper.h
    include/dsfw/IStepPlugin.h
    include/dsfw/IPageActions.h
    include/dsfw/IPageLifecycle.h
    include/dsfw/IPageProgress.h
    include/dsfw/IconNavBar.h
    include/dsfw/AppShell.h
    include/dsfw/PluginManager.h
)

set(DSFW_UI_CORE_RESOURCES
    res/ui-core.qrc
)

add_library(${PROJECT_NAME} STATIC ${DSFW_UI_CORE_SOURCES} ${DSFW_UI_CORE_HEADERS} ${DSFW_UI_CORE_RESOURCES})
add_library(dsfw::ui-core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

find_package(QWindowKit CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC QWindowKit::Widgets)

# Install rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT dsfwTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(FILES ${DSFW_UI_CORE_RESOURCES}
    DESTINATION "${CMAKE_INSTALL_DATADIR}/dsfw"
)
