project(dstools-widgets)

set(CMAKE_AUTOMOC ON)

set(WIDGETS_SOURCES
    src/PlayWidget.cpp
    src/TaskWindow.cpp
    src/GpuSelector.cpp
    src/ShortcutEditorWidget.cpp
)

set(WIDGETS_HEADERS
    include/dstools/WidgetsGlobal.h
    include/dstools/PlayWidget.h
    include/dstools/TaskWindow.h
    include/dstools/GpuSelector.h
    include/dstools/ShortcutEditorWidget.h
)

add_library(${PROJECT_NAME} SHARED ${WIDGETS_SOURCES} ${WIDGETS_HEADERS})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-core
    dstools-audio
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE DSTOOLS_WIDGETS_BUILD)

# Windows: link dxgi for GpuSelector
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dxgi)
endif()
