project(LabelSuite VERSION 0.1.0.0)

find_package(cpp-pinyin CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    SOURCES main.cpp FileDataSource.h FileDataSource.cpp
    DEPENDS
        PRIVATE dstools-ui-core minlabel-page phonemelabeler-page pitchlabeler-page
)

# Copy cpp-pinyin dictionary (needed for MinLabel G2P)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
    ${CMAKE_BINARY_DIR}/bin/dict
)
install(DIRECTORY ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
    DESTINATION .
)
