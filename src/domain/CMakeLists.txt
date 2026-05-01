project(dstools-domain)

set(CMAKE_AUTOMOC ON)

set(DOMAIN_SOURCES
    src/CsvToDsConverter.cpp
    src/DsDocument.cpp
    src/ExportFormats.cpp
    src/PitchUtils.cpp
    src/AudioFileResolver.cpp
    src/F0Curve.cpp
    src/PhNumCalculator.cpp
    src/TextGridToCsv.cpp
    src/TranscriptionCsv.cpp
    src/TranscriptionPipeline.cpp
    src/DsDocumentAdapter.cpp
    src/DsItemManager.cpp
    src/DsProject.cpp
    src/PinyinG2PProvider.cpp
    src/QualityMetrics.cpp
    src/ModelManager.cpp
    src/ModelDownloader.cpp
)

set(DOMAIN_HEADERS
    include/dstools/CsvToDsConverter.h
    include/dstools/DsDocument.h
    include/dstools/PhNumCalculator.h
    include/dstools/PitchUtils.h
    include/dstools/AudioFileResolver.h
    include/dstools/F0Curve.h
    include/dstools/TextGridToCsv.h
    include/dstools/TranscriptionCsv.h
    include/dstools/TranscriptionPipeline.h
    include/dstools/DsDocumentAdapter.h
    include/dstools/ExportFormats.h
    include/dstools/PinyinG2PProvider.h
    include/dstools/QualityMetrics.h
    include/dstools/DsItemManager.h
    include/dstools/DsProject.h
    include/dstools/ModelManager.h
    include/dstools/ModelDownloader.h
    include/dstools/StringUtils.h
    include/dstools/PathUtils.h
)

add_library(${PROJECT_NAME} STATIC ${DOMAIN_SOURCES} ${DOMAIN_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    dstools-types
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

target_link_libraries(${PROJECT_NAME} PRIVATE textgrid)

find_package(SndFile CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SndFile::sndfile)
