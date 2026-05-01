project(dstools-widgets)

set(CMAKE_AUTOMOC ON)

set(WIDGETS_SOURCES
    src/GpuSelector.cpp
    src/FileStatusDelegate.cpp
    src/BaseFileListPanel.cpp
    src/ModelLoadPanel.cpp
)

set(WIDGETS_HEADERS
    include/dstools/WidgetsGlobal.h
    include/dstools/GpuSelector.h
    include/dstools/FileStatusDelegate.h
    include/dstools/BaseFileListPanel.h
    include/dstools/ModelLoadPanel.h
)

add_library(${PROJECT_NAME} SHARED ${WIDGETS_SOURCES} ${WIDGETS_HEADERS})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-widgets
    dstools-ui-core
    dsfw-core
    dstools-audio
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE DSTOOLS_WIDGETS_BUILD)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dxgi)
endif()
