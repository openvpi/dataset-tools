project(dsfw-core)

if (NOT TARGET dsfw::types)
    find_package(dsfw-types CONFIG REQUIRED)
endif ()

if (NOT TARGET Qt6::Core)
    find_package(QT NAMES Qt6 COMPONENTS Core Network Widgets REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Widgets REQUIRED)
endif ()

find_package(nlohmann_json CONFIG REQUIRED)

configure_file(
        include/dsfw/Version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/dsfw/Version.h
        @ONLY
)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dsfw::core
        EXPORT_SET dsfwTargets
        DEPENDS
        PUBLIC dsfw::types Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network nlohmann_json::nlohmann_json
        PRIVATE $<$<PLATFORM_ID:Windows>:dbghelp> Qt${QT_VERSION_MAJOR}::Widgets
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

dstools_add_translations(${PROJECT_NAME} EXPORT_SET dsfwTargets)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /wd4275)
endif ()

# Install generated Version.h
if (NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/dsfw/Version.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/dsfw"
    )
endif ()
