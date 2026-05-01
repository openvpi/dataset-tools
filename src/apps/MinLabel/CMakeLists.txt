project(MinLabel VERSION 0.1.0.0)

add_subdirectory(gui)

find_package(cpp-pinyin CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    SOURCES main.cpp MinLabelKeys.h res.qrc
    DEPENDS
        PRIVATE minlabel-page dstools-ui-core
)

# Copy cpp-pinyin dictionary
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
    ${CMAKE_BINARY_DIR}/bin/dict
)
install(DIRECTORY ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
    DESTINATION .
)
