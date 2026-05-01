project(pipeline-slicer-page)

set(CMAKE_AUTOMOC ON)

set(SLICER_PAGE_SOURCES
        SlicerPage.cpp
        WorkThread.cpp
        SliceJob.cpp
        MarkerIO.cpp
        AudioFileLoader.cpp
        AudioChunkWriter.cpp
)

set(SLICER_PAGE_HEADERS
        SlicerPage.h
        WorkThread.h
        SliceJob.h
        MarkerIO.h
        AudioFileLoader.h
        AudioChunkWriter.h
)

add_library(${PROJECT_NAME} STATIC ${SLICER_PAGE_SOURCES} ${SLICER_PAGE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(SndFile CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
        dsfw-ui-core
    dstools-widgets
        dstools-audio
        slicer-lib
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
)
