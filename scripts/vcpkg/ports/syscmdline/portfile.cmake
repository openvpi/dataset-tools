vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO SineStriker/syscmdline
    REF 0c9f3de8b11bd2f33b03bea5521bf446af4ead69
    SHA512 0268dd94bad848e4d05957327d3149c13e2459fb8e82a7e8577ec464108bf69f8d7719df847194c1b6efc8edacd8353c26444e978b40e114b4e4e0ab06b5c0bf
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ${PORT} CONFIG_PATH lib/cmake/${PORT})
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)