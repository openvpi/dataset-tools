project(dstools-ui-core VERSION 3.0.0)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC AUTORCC
        NAMESPACE dstools::ui-core
        EXPORT_SET dstoolsUiCoreTargets
        RESOURCES ${CMAKE_SOURCE_DIR}/resources/icons.res.qrc
        DEPENDS
        PUBLIC dsfw-ui-core dsfw-core dstools-domain Qt${QT_VERSION_MAJOR}::Widgets
)

# Install export set and package config
if (NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(EXPORT dstoolsUiCoreTargets
            FILE dstoolsUiCoreTargets.cmake
            NAMESPACE dstools::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dstools-ui-core"
    )

    configure_package_config_file(
            "${PROJECT_CMAKE_MODULES_DIR}/dstools-ui-coreConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-ui-coreConfig.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dstools-ui-core"
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-ui-coreConfigVersion.cmake"
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-ui-coreConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/dstools-ui-coreConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dstools-ui-core"
    )
endif ()
