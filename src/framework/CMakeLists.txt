# dsfw framework version
set(DSFW_VERSION_MAJOR 1)
set(DSFW_VERSION_MINOR 0)
set(DSFW_VERSION_PATCH 0)
set(DSFW_VERSION "${DSFW_VERSION_MAJOR}.${DSFW_VERSION_MINOR}.${DSFW_VERSION_PATCH}")

add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(ui-core)
add_subdirectory(audio)
add_subdirectory(infer)

# === dsfw package export/install ===
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(EXPORT dsfwTargets
    FILE dsfwTargets.cmake
    NAMESPACE dsfw::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dsfw"
)

configure_package_config_file(
    "${PROJECT_CMAKE_MODULES_DIR}/dsfwConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dsfwConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dsfw"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/dsfwConfigVersion.cmake"
    VERSION ${DSFW_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dsfwConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dsfwConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dsfw"
)
