project(dsfw-core)

set(CMAKE_AUTOMOC ON)

set(DSFW_CORE_SOURCES
    src/AppSettings.cpp
    src/AsyncTask.cpp
    src/JsonHelper.cpp
    src/ServiceLocator.cpp
    src/LocalFileIOProvider.cpp
)

set(DSFW_CORE_HEADERS
    include/dsfw/AppSettings.h
    include/dsfw/AsyncTask.h
    include/dsfw/JsonHelper.h
    include/dsfw/ServiceLocator.h
    include/dsfw/IFileIOProvider.h
    include/dsfw/LocalFileIOProvider.h
    include/dsfw/IDocument.h
    include/dsfw/IExportFormat.h
    include/dsfw/IModelProvider.h
    include/dsfw/IModelDownloader.h
    include/dsfw/IQualityMetrics.h
    include/dsfw/IG2PProvider.h
)

add_library(${PROJECT_NAME} STATIC ${DSFW_CORE_SOURCES} ${DSFW_CORE_HEADERS})
add_library(dsfw::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-types
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
