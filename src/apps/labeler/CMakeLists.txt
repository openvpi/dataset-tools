project(DiffSingerLabeler VERSION 0.1.0.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME}
    main.cpp
    CleanupDialog.cpp
    CleanupDialog.h
    LabelerWindow.cpp
    LabelerWindow.h
    StepNavigator.cpp
    StepNavigator.h
    IPageActions.h
    IPageLifecycle.h
    IPageProgress.h
    pages/BuildCsvPage.cpp
    pages/BuildCsvPage.h
    pages/GameAlignPage.cpp
    pages/GameAlignPage.h
    pages/BuildDsPage.cpp
    pages/BuildDsPage.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dstools-widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

# Include directories for embedded Page sources
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/hubertfa
)

# MinLabel Page sources
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/MinLabelPage.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/MinLabelPage.h
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/TextWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/TextWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/Common.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/Common.h
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/ExportDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/MinLabel/gui/ExportDialog.h
)

# PhonemeLabeler Page sources
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/PhonemeLabelerKeys.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/PhonemeLabelerPage.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/PhonemeLabelerPage.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/TextGridDocument.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/TextGridDocument.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/BoundaryBindingManager.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/BoundaryBindingManager.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/BoundaryOverlayWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/BoundaryOverlayWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/EntryListPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/EntryListPanel.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/FileListPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/FileListPanel.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/IntervalTierView.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/IntervalTierView.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/PowerWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/PowerWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/SpectrogramColorPalette.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/SpectrogramWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/SpectrogramWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/TierEditWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/TierEditWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/TimeRulerWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/TimeRulerWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/WaveformWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/WaveformWidget.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/InsertBoundaryCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/InsertBoundaryCommand.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/LinkedMoveBoundaryCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/LinkedMoveBoundaryCommand.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/MoveBoundaryCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/MoveBoundaryCommand.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/RemoveBoundaryCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/RemoveBoundaryCommand.h
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/SetIntervalTextCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PhonemeLabeler/gui/ui/commands/SetIntervalTextCommand.h
)

# PitchLabeler Page sources
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/PitchLabelerKeys.h
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/PitchLabelerPage.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/PitchLabelerPage.h
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/DSFile.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/DSFile.h
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/FileListPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/FileListPanel.h
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/PianoRollView.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/PianoRollView.h
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/PropertyPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/PropertyPanel.h
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/commands/PitchCommands.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/PitchLabeler/gui/ui/commands/PitchCommands.h
)

# Pipeline Page sources (Slicer, LyricFA, HubertFA)
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/SlicerPage.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/SlicerPage.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/Slicer.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/Slicer.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/WorkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/WorkThread.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/MathUtils.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/slicer/Enumerations.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricFAPage.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricFAPage.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/AsrTask.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/AsrTask.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricMatchTask.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricMatchTask.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/Asr.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/Asr.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/MatchLyric.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/MatchLyric.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricMatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricMatcher.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/SequenceAligner.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/SequenceAligner.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/ChineseProcessor.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/ChineseProcessor.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/SmartHighlighter.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/SmartHighlighter.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/LyricData.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/Utils.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/lyricfa/Utils.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/hubertfa/HubertFAPage.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/hubertfa/HubertFAPage.h
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/hubertfa/HfaTask.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/pipeline/hubertfa/HfaTask.h
)

# Additional libraries for embedded Pages
find_package(cpp-pinyin CONFIG REQUIRED)
find_package(cpp-kana CONFIG REQUIRED)
find_package(FFTW3 CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dstools-audio
    audio-util
    hubert-infer
    FunAsr
    cpp-pinyin::cpp-pinyin
    cpp-kana::cpp-kana
    textgrid
    FFTW3::fftw3
    SndFile::sndfile
    nlohmann_json::nlohmann_json
)

if(WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)
    if(NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})
