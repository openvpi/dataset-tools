project(lyricfa-lib)

set(CMAKE_AUTOMOC ON)

set(LYRICFA_SOURCES
    Asr.cpp
    AsrTask.cpp
    ChineseProcessor.cpp
    FunAsrAdapter.cpp
    FunAsrProcessor.cpp
    LyricMatcher.cpp
    LyricMatchTask.cpp
    MatchLyric.cpp
    SequenceAligner.cpp
    SmartHighlighter.cpp
    Utils.cpp
)

set(LYRICFA_HEADERS
    Asr.h
    AsrTask.h
    ChineseProcessor.h
    FunAsrAdapter.h
    FunAsrProcessor.h
    LyricData.h
    LyricMatcher.h
    LyricMatchTask.h
    MatchLyric.h
    SequenceAligner.h
    SmartHighlighter.h
    Utils.h
)

add_library(${PROJECT_NAME} STATIC ${LYRICFA_SOURCES} ${LYRICFA_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    dstools-infer-common
    audio-util
    FunAsr
    cpp-pinyin::cpp-pinyin
    SndFile::sndfile
    nlohmann_json::nlohmann_json
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
