project(dsfw-core)

set(CMAKE_AUTOMOC ON)

set(DSFW_CORE_SOURCES
    src/AppSettings.cpp
    src/AsyncTask.cpp
    src/JsonHelper.cpp
    src/ServiceLocator.cpp
    src/LocalFileIOProvider.cpp
    src/ModelDownloader.cpp
    src/ModelManager.cpp
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
    include/dsfw/CommonKeys.h
    include/dsfw/ModelDownloader.h
    include/dsfw/ModelManager.h
)

add_library(${PROJECT_NAME} STATIC ${DSFW_CORE_SOURCES} ${DSFW_CORE_HEADERS})
add_library(dsfw::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-types::dstools-types
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

# Install rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT dsfwTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
