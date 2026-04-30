add_subdirectory(core)
add_subdirectory(ui-core)

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
    VERSION 0.1.0
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dsfwConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/dsfwConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/dsfw"
)
