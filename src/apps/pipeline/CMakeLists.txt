project(DatasetPipeline VERSION 1.0.0.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(PIPELINE_SOURCES
    main.cpp
    PipelineWindow.h    PipelineWindow.cpp

    # Slicer Tab
    slicer/SlicerPage.h      slicer/SlicerPage.cpp
    slicer/Slicer.h           slicer/Slicer.cpp
    slicer/WorkThread.h       slicer/WorkThread.cpp
    slicer/MathUtils.h
    slicer/Enumerations.h

    # LyricFA Tab
    lyricfa/LyricFAPage.h     lyricfa/LyricFAPage.cpp
    lyricfa/AsrTask.h          lyricfa/AsrTask.cpp
    lyricfa/LyricMatchTask.h   lyricfa/LyricMatchTask.cpp
    lyricfa/Asr.h              lyricfa/Asr.cpp
    lyricfa/MatchLyric.h       lyricfa/MatchLyric.cpp
    lyricfa/LyricMatcher.h     lyricfa/LyricMatcher.cpp
    lyricfa/SequenceAligner.h  lyricfa/SequenceAligner.cpp
    lyricfa/ChineseProcessor.h lyricfa/ChineseProcessor.cpp
    lyricfa/SmartHighlighter.h lyricfa/SmartHighlighter.cpp
    lyricfa/LyricData.h
    lyricfa/Utils.h            lyricfa/Utils.cpp

    # HubertFA Tab
    hubertfa/HubertFAPage.h   hubertfa/HubertFAPage.cpp
    hubertfa/HfaTask.h         hubertfa/HfaTask.cpp
)

add_executable(${PROJECT_NAME} WIN32 ${PIPELINE_SOURCES})

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/slicer
    ${CMAKE_CURRENT_SOURCE_DIR}/lyricfa
    ${CMAKE_CURRENT_SOURCE_DIR}/hubertfa
    ${CMAKE_CURRENT_SOURCE_DIR}/../labeler
)

find_package(SndFile CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dstools-widgets
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

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

if(WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)
endif()

if(ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ONNXRUNTIME_ENABLE_DML)
    target_link_libraries(${PROJECT_NAME} PRIVATE dxgi)
endif()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})
