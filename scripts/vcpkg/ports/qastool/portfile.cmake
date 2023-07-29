vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO SineStriker/qt-json-autogen
    REF 0.0.2.3
    SHA512 e98b4b21c640c4d31ef9f2e30fed263b0cb5c85b2c467d37835e0fe9337d733c92b9dcf1dc9e252550209942fa0248755c5e3b708b630e68463869fa541e9a3e
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DQAS_BUILD_EXAMPLES=OFF
        -DQAS_BUILD_MOC_EXE=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME qastool
    CONFIG_PATH lib/cmake/${PORT}
)

if(APPLE)
  get_filename_component(qt_lib_dir "$ENV{QT_DIR}/../.." REALPATH)
  execute_process(COMMAND install_name_tool -add_rpath "${qt_lib_dir}" "${CURRENT_PACKAGES_DIR}/tools/qastool/qasc")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug")
file(COPY "${CURRENT_PACKAGES_DIR}/tools" DESTINATION "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/" RENAME copyright)