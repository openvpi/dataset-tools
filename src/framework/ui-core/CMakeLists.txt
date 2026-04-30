project(dsfw-ui-core)

set(CMAKE_AUTOMOC ON)

set(DSFW_UI_CORE_SOURCES)
set(DSFW_UI_CORE_HEADERS)

add_library(${PROJECT_NAME} STATIC ${DSFW_UI_CORE_SOURCES} ${DSFW_UI_CORE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
