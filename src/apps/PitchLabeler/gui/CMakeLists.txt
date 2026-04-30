project(pitchlabeler-page)

set(CMAKE_AUTOMOC ON)

set(PAGE_SOURCES
    PitchLabelerPage.cpp
    PitchLabelerPage.h
    DSFile.cpp
    DSFile.h
    ui/FileListPanel.cpp
    ui/FileListPanel.h
    ui/PianoRollView.cpp
    ui/PianoRollView.h
    ui/PropertyPanel.cpp
    ui/PropertyPanel.h
    ui/commands/PitchCommands.cpp
    ui/commands/PitchCommands.h
)

add_library(${PROJECT_NAME} STATIC ${PAGE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/commands>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
