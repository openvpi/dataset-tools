project(dsfw-core)

set(CMAKE_AUTOMOC ON)

if(NOT TARGET dstools-types::dstools-types)
    find_package(dstools-types CONFIG REQUIRED)
endif()

if(NOT TARGET Qt6::Core)
    find_package(QT NAMES Qt6 COMPONENTS Core Network REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network REQUIRED)
endif()

configure_file(
    include/dsfw/Version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/dsfw/Version.h
    @ONLY
)

set(DSFW_CORE_SOURCES
    src/AppSettings.cpp
    src/AsyncTask.cpp
    src/JsonHelper.cpp
    src/ServiceLocator.cpp
    src/LocalFileIOProvider.cpp
    src/Log.cpp
    src/UndoStack.cpp
    src/EventBus.cpp
    src/UpdateChecker.cpp
    src/RecentFiles.cpp
    src/CrashHandler.cpp
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
    include/dsfw/IModelManager.h
    include/dsfw/IQualityMetrics.h
    include/dsfw/IG2PProvider.h
    include/dsfw/ISlicerService.h
    include/dsfw/IAlignmentService.h
    include/dsfw/IAsrService.h
    include/dsfw/IPitchService.h
    include/dsfw/ITranscriptionService.h
    include/dsfw/InferenceModelProvider.h
    include/dsfw/CommonKeys.h
    include/dsfw/Log.h
    include/dsfw/UndoStack.h
    include/dsfw/EventBus.h
    include/dsfw/UpdateChecker.h
    include/dsfw/RecentFiles.h
    include/dsfw/CrashHandler.h
)

add_library(${PROJECT_NAME} STATIC ${DSFW_CORE_SOURCES} ${DSFW_CORE_HEADERS})
add_library(dsfw::core ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-base
    dstools-types::dstools-types
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dbghelp)
endif()

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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/dsfw/Version.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/dsfw"
)
