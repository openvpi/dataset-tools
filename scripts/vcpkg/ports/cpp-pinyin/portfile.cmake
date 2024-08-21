vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO wolfgitpr/cpp-pinyin
        REF  c6fb0a9fd706fe4a5bb6c9bae614561ca7b5c043
        SHA512 aaa5247268e8f74544e09f1c611dbb42eff0db1a3dce6dbdaac74893f6a01bb25ecadc16d229ae7972dfb23de42c01337ff393279c5a03bba258da4a07f3cb9e
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