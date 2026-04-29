project(dstools-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CORE_SOURCES
    src/AppInit.cpp
    src/AppSettings.cpp
    src/CsvToDsConverter.cpp
    src/DsDocument.cpp
    src/JsonHelper.cpp
    src/Theme.cpp
    src/FramelessHelper.cpp
    src/PitchUtils.cpp
    src/AudioFileResolver.cpp
    src/F0Curve.cpp
    src/PhNumCalculator.cpp
    src/TextGridToCsv.cpp
    src/TranscriptionCsv.cpp
    src/TranscriptionPipeline.cpp
    src/LocalFileIOProvider.cpp
    src/DsDocumentAdapter.cpp
    src/ModelManager.cpp
    src/DsItemManager.cpp
    src/DsProject.cpp
)

set(CORE_HEADERS
    include/dstools/AppInit.h
    include/dstools/AppSettings.h
    include/dstools/CommonKeys.h
    include/dstools/CsvToDsConverter.h
    include/dstools/DsDocument.h
    include/dstools/JsonHelper.h
    include/dstools/Theme.h
    include/dstools/FramelessHelper.h
    include/dstools/PhNumCalculator.h
    include/dstools/PitchUtils.h
    include/dstools/AudioFileResolver.h
    include/dstools/F0Curve.h
    include/dstools/TextGridToCsv.h
    include/dstools/TranscriptionCsv.h
    include/dstools/TranscriptionPipeline.h
    include/dstools/IFileIOProvider.h
    include/dstools/LocalFileIOProvider.h
    include/dstools/IDocument.h
    include/dstools/DsDocumentAdapter.h
    include/dstools/IExportFormat.h
    include/dstools/IG2PProvider.h
    include/dstools/IModelDownloader.h
    include/dstools/IQualityMetrics.h
    include/dstools/IStepPlugin.h
    include/dstools/IModelProvider.h
    include/dstools/ModelManager.h
    include/dstools/DsItemManager.h
    include/dstools/DsProject.h
)

set(CORE_RESOURCES
    res/core.qrc
)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES} ${CORE_HEADERS} ${CORE_RESOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# JSON support for AppSettings
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

# QWindowKit for frameless windows
find_package(QWindowKit CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC QWindowKit::Widgets)

# Optional: qBreakpad support
find_package(qBreakpad CONFIG QUIET)
if(qBreakpad_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE qBreakpad::qBreakpad)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_QBREAKPAD)
endif()

# Optional: cpp-pinyin support
find_package(cpp-pinyin CONFIG QUIET)
if(cpp-pinyin_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE cpp-pinyin::cpp-pinyin)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_CPP_PINYIN)
endif()

# textgrid (header-only) for TextGridToCsv
target_link_libraries(${PROJECT_NAME} PRIVATE textgrid)
