project(dstools-core)

set(CMAKE_AUTOMOC ON)

set(CORE_SOURCES
    src/AppSettings.cpp
    src/CsvToDsConverter.cpp
    src/DsDocument.cpp
    src/JsonHelper.cpp
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
    src/ServiceLocator.cpp
    src/PinyinG2PProvider.cpp
    src/ModelDownloader.cpp
    src/QualityMetrics.cpp
)

set(CORE_HEADERS
    include/dstools/AppSettings.h
    include/dstools/CommonKeys.h
    include/dstools/ServiceLocator.h
    include/dstools/CsvToDsConverter.h
    include/dstools/DsDocument.h
    include/dstools/JsonHelper.h
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
    include/dstools/PinyinG2PProvider.h
    include/dstools/IModelDownloader.h
    include/dstools/ModelDownloader.h
    include/dstools/IQualityMetrics.h
    include/dstools/QualityMetrics.h
    include/dstools/IModelProvider.h
    include/dstools/ModelManager.h
    include/dstools/DsItemManager.h
    include/dstools/DsProject.h
)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-types
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)

# JSON support for AppSettings
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

# textgrid (header-only) for TextGridToCsv
target_link_libraries(${PROJECT_NAME} PRIVATE textgrid)

# SndFile for built-in F0 extraction fallback
find_package(SndFile CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SndFile::sndfile)
