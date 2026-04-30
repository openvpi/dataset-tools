project(pipeline-pages)

set(CMAKE_AUTOMOC ON)

set(PAGE_SOURCES
    # Slicer
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/SlicerPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/SlicerPage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/WorkThread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/WorkThread.h

    # LyricFA
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricFAPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricFAPage.h

    # HubertFA
    ${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa/HubertFAPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa/HubertFAPage.h
)

add_library(${PROJECT_NAME} STATIC ${PAGE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../slicer>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa>
)

find_package(SndFile CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-widgets
    labeler-interfaces
    audio-util
    hubert-infer
    slicer-lib
    hubertfa-lib
    lyricfa-lib
    FunAsr
    textgrid
    cpp-pinyin::cpp-pinyin
    SndFile::sndfile
    nlohmann_json::nlohmann_json
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
