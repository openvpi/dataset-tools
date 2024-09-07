vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO wolfgitpr/cpp-pinyin
        REF  2c87514e809e878bce5b1b3661b96033c807e6d6
        SHA512 7d5143d215789779c531523a7ac44773f3acd3de0f354471c559e78f96bc7a60883c059f721d80d7d1980a215775ad255db11c64c704b2177847f09e5a311dd2
        HEAD_REF main
)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DCPP_PINYIN_BUILD_STATIC=FALSE
        -DCPP_PINYIN_BUILD_TESTS=FALSE
        -DVCPKG_DICT_DIR=${CURRENT_PACKAGES_DIR}/share/${PORT}
)

vcpkg_cmake_install()

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/share")
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/share/${PORT}")

vcpkg_cmake_config_fixup(PACKAGE_NAME ${PORT} CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)