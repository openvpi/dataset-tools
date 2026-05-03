project(DsLabeler VERSION 0.1.0.0)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECETABLE MACOSX_BUNDLE WINRC
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets dstools-domain
                min-label-editor phoneme-editor pitch-editor waveform-panel
                lyricfa-lib
                SndFile::sndfile
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
)

dstools_add_translations(${PROJECT_NAME})

# Copy cpp-pinyin dictionary (needed for MinLabel G2P)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
    ${CMAKE_BINARY_DIR}/bin/dict
)
install(DIRECTORY ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
    DESTINATION .
)
