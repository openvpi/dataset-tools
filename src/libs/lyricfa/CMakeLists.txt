project(lyricfa-lib)

set(CMAKE_AUTOMOC ON)

set(LYRICFA_SOURCES
    Asr.cpp
    AsrTask.cpp
    ChineseProcessor.cpp
    LyricMatcher.cpp
    LyricMatchTask.cpp
    MatchLyric.cpp
    SequenceAligner.cpp
    Utils.cpp
)

set(LYRICFA_HEADERS
    Asr.h
    AsrTask.h
    ChineseProcessor.h
    LyricData.h
    LyricMatcher.h
    LyricMatchTask.h
    MatchLyric.h
    SequenceAligner.h
    Utils.h
)

add_library(${PROJECT_NAME} STATIC ${LYRICFA_SOURCES} ${LYRICFA_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    FunAsr
    SndFile::sndfile
    nlohmann_json::nlohmann_json
    Qt${QT_VERSION_MAJOR}::Core
)
