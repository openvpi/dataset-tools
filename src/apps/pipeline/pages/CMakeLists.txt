project(pipeline-pages)

set(CMAKE_AUTOMOC ON)

set(PAGE_SOURCES
    # Slicer
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/SlicerPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/SlicerPage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/Slicer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/Slicer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/WorkThread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/WorkThread.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/MathUtils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../slicer/Enumerations.h

    # LyricFA
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricFAPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricFAPage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/AsrTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/AsrTask.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricMatchTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricMatchTask.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/Asr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/Asr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/MatchLyric.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/MatchLyric.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricMatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/SequenceAligner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/SequenceAligner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/ChineseProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/ChineseProcessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/SmartHighlighter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/SmartHighlighter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/LyricData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/Utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../lyricfa/Utils.h

    # HubertFA
    ${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa/HubertFAPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa/HubertFAPage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa/HfaTask.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../hubertfa/HfaTask.h
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
    FunAsr
    textgrid
    cpp-pinyin::cpp-pinyin
    SndFile::sndfile
    nlohmann_json::nlohmann_json
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
