vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO wolfgitpr/cpp-pinyin
        REF 392463174391c97c70701f5144646dac22ff20a8
        SHA512 2a94a92d0ed920a7d693c425eac9c248cee9d3650ae3b9e26124da707a78e8a7943b8f6aa247b10ec2e1d8fd184ea142e32e3508d3effd8a583f32118301209e
        HEAD_REF main
)

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "static" CPP_PINYIN_BUILD_STATIC)

vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        OPTIONS
        -DCPP_PINYIN_BUILD_STATIC=${CPP_PINYIN_BUILD_STATIC}
        -DCPP_PINYIN_BUILD_TESTS=FALSE
        -DCPP_PINYIN_VCPKG_DICT_DIR=${CURRENT_PACKAGES_DIR}/share/${PORT}
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/${PORT}")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
configure_file("${CMAKE_CURRENT_LIST_DIR}/usage" "${CURRENT_PACKAGES_DIR}/share/${PORT}/usage" COPYONLY)
