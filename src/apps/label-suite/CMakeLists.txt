project(LabelSuite VERSION 0.1.0.0)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    SOURCES
        main.cpp
        TaskWindowAdapter.h
        TaskWindowAdapter.cpp
        CleanupDialog.h
        CleanupDialog.cpp
        commands/DiscardSliceCommand.h
        commands/DiscardSliceCommand.cpp
        pages/BuildCsvPage.h
        pages/BuildCsvPage.cpp
        pages/BuildDsPage.h
        pages/BuildDsPage.cpp
        pages/GameAlignPage.h
        pages/GameAlignPage.cpp
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets minlabel-page phonemelabeler-page
                pitchlabeler-page pipeline-pages game-infer rmvpe-infer
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/pages
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
