project(dsfw-ui-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(DSFW_UI_CORE_SOURCES
    src/Theme.cpp
    src/FramelessHelper.cpp
)

set(DSFW_UI_CORE_HEADERS
    include/dsfw/Theme.h
    include/dsfw/FramelessHelper.h
    include/dsfw/IStepPlugin.h
)

set(DSFW_UI_CORE_RESOURCES
    res/ui-core.qrc
)

add_library(${PROJECT_NAME} STATIC ${DSFW_UI_CORE_SOURCES} ${DSFW_UI_CORE_HEADERS} ${DSFW_UI_CORE_RESOURCES})
add_library(dsfw::ui-core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

find_package(QWindowKit CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC QWindowKit::Widgets)
