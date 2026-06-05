project(textgrid)

dstools_add_library(${PROJECT_NAME}
    INTERFACE
    NAMESPACE textgrid::textgrid
    EXPORT_SET ${PROJECT_NAME}Targets
)

if (NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )
endif()
