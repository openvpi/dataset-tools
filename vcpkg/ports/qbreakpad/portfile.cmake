vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO stdware/qBreakpad
    REF 22a386292ff6304fef3ed1ef46158e4a37ac7339
    SHA512 c0fbd0ad62132883592423641900a608ffb0fdb5093e3dfaa91dd56ba33ea6453dcb3f0ca2c4a31c50181630b36afaf7c597c1b8dde04bf67a03aa0bd8d8ac02
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DQBREAKPAD_BUILD_SHARED=TRUE
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME ${PORT} CONFIG_PATH lib/cmake/qBreakpad)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE.LGPL" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/" RENAME copyright)