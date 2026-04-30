project(pipeline-slicer-page)

set(CMAKE_AUTOMOC ON)

set(SLICER_PAGE_SOURCES
    SlicerPage.cpp
    WorkThread.cpp
)

set(SLICER_PAGE_HEADERS
    SlicerPage.h
    WorkThread.h
)

add_library(${PROJECT_NAME} STATIC ${SLICER_PAGE_SOURCES} ${SLICER_PAGE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(SndFile CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-widgets
    labeler-interfaces
    slicer-lib
    audio-util
    SndFile::sndfile
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
