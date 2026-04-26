set(LIB_PREFIX "https://github.com/SineStriker/ffmpeg-fake/releases/download")
set(LIB_VERSION 0.0.5)

if(WIN32)
    set(FILE_NAME ffmpeg-fake-windows-amd64-${LIB_VERSION}.zip)
    set(FILE_CHECK_SUM 05c3582cf9dba7478b498570ab6d2c7a776d53e51542263cc37a270c6b2f6a08e8a8b52d5d439a2d2d89274cc2d57224c5bc93a5e1beebf6ab41aeb399469d9c)
elseif(APPLE)
    if(VCPKG_OSX_ARCHITECTURES STREQUAL arm64)
        set(FILE_NAME ffmpeg-fake-mac-arm64-${LIB_VERSION}.zip)
        set(FILE_CHECK_SUM 37b53104aa0c685c4e946080bb228ef6cf534c7ed2aebd3b472bca0c2e4989c83eb650c4a9f75aeaa111f1aebd6d8514964f6824577f15f8576e3931de874808)
    else()
        set(FILE_NAME ffmpeg-fake-mac-amd64-${LIB_VERSION}.zip)
        set(FILE_CHECK_SUM ac58101c9b5e94056c59ba8d114b1197872bc3cf602519cf2931bbfd860ff8a8d488733089e6e25e45508fc77f43ede8440d087a5e43603bb20716c880c47bdb)
    endif()
else()
    set(FILE_NAME ffmpeg-fake-linux-amd64-${LIB_VERSION}.zip)
    set(FILE_CHECK_SUM ac79eb418d101ddd17f48a3ed1b4b47dbb990ed723920a09d9eadcaf2c8a26010ad4d57b4fa1ec15ffdce470d18ddf9941cb18faa920c78fea52cbf1c99beaac)
endif()

vcpkg_download_distfile(ARCHIVE
    URLS ${LIB_PREFIX}/${LIB_VERSION}/${FILE_NAME}
    FILENAME ${FILE_NAME}
    SHA512 ${FILE_CHECK_SUM}
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"

    # NO_REMOVE_ONE_LEVEL
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
    -DCMAKE_INSTALL_INCLUDEDIR=${CURRENT_PACKAGES_DIR}/include
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/ffmpeg-fake)
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/" RENAME copyright)
