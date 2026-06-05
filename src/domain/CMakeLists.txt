project(dstools-domain VERSION 3.0.0)

set(DSTOOLS_DOMAIN_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(DSTOOLS_DOMAIN_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(DSTOOLS_DOMAIN_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(DSTOOLS_DOMAIN_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

find_package(nlohmann_json CONFIG REQUIRED)

configure_file(
        include/dstools/Version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/dstools/Version.h
        @ONLY
)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::domain
        EXPORT_SET dstoolsDomainTargets
        DEPENDS
        PUBLIC dsfw-core dstools-types
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network nlohmann_json::nlohmann_json
        PRIVATE textgrid dsfw-audio
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

# Install export set and package config
if (NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(EXPORT dstoolsDomainTargets
            FILE dstoolsDomainTargets.cmake
            NAMESPACE dstools::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dstools-domain"
    )

    configure_package_config_file(
            "${PROJECT_CMAKE_MODULES_DIR}/dstools-domainConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-domainConfig.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dstools-domain"
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-domainConfigVersion.cmake"
            VERSION ${DSTOOLS_DOMAIN_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-domainConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-domainConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dstools-domain"
    )
endif ()
