project(phonemelabeler-page)

set(CMAKE_AUTOMOC ON)

set(PAGE_SOURCES
    PhonemeLabelerPage.cpp
    PhonemeLabelerPage.h
    ui/TextGridDocument.cpp
    ui/TextGridDocument.h
    ui/BoundaryBindingManager.cpp
    ui/BoundaryBindingManager.h
    ui/BoundaryOverlayWidget.cpp
    ui/BoundaryOverlayWidget.h
    ui/EntryListPanel.cpp
    ui/EntryListPanel.h
    ui/FileListPanel.cpp
    ui/FileListPanel.h
    ui/IntervalTierView.cpp
    ui/IntervalTierView.h
    ui/PowerWidget.cpp
    ui/PowerWidget.h
    ui/SpectrogramColorPalette.h
    ui/SpectrogramWidget.cpp
    ui/SpectrogramWidget.h
    ui/TierEditWidget.cpp
    ui/TierEditWidget.h
    ui/TimeRulerWidget.cpp
    ui/TimeRulerWidget.h
    ui/WaveformWidget.cpp
    ui/WaveformWidget.h
    ui/commands/InsertBoundaryCommand.cpp
    ui/commands/InsertBoundaryCommand.h
    ui/commands/LinkedMoveBoundaryCommand.cpp
    ui/commands/LinkedMoveBoundaryCommand.h
    ui/commands/MoveBoundaryCommand.cpp
    ui/commands/MoveBoundaryCommand.h
    ui/commands/RemoveBoundaryCommand.cpp
    ui/commands/RemoveBoundaryCommand.h
    ui/commands/SetIntervalTextCommand.cpp
    ui/commands/SetIntervalTextCommand.h
)

add_library(${PROJECT_NAME} STATIC ${PAGE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/commands>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)

find_package(FFTW3 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-widgets
    dstools-audio
    textgrid
    FFTW3::fftw3
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
