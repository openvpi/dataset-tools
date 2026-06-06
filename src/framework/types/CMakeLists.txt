project(dsfw-types)

dstools_add_library(${PROJECT_NAME}
        INTERFACE
        NAMESPACE dsfw::types
        EXPORT_SET ${PROJECT_NAME}Targets
)

# Package config for standalone consumption
if (NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(EXPORT ${PROJECT_NAME}Targets
            FILE ${PROJECT_NAME}Targets.cmake
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    configure_package_config_file(
            "${PROJECT_CMAKE_MODULES_DIR}/dsfw-typesConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/dsfw-typesConfig.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/dsfw-typesConfig.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )
endif ()
