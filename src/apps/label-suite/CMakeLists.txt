project(LabelSuite VERSION 0.1.0.0)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)

# LabelSuite shares source files with DiffSingerLabeler (TaskWindowAdapter,
# CleanupDialog, DiscardSliceCommand, BuildCsvPage, BuildDsPage, GameAlignPage).
set(_labeler_dir ${CMAKE_CURRENT_SOURCE_DIR}/../labeler)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    SOURCES
        main.cpp
        ${_labeler_dir}/TaskWindowAdapter.h
        ${_labeler_dir}/TaskWindowAdapter.cpp
        ${_labeler_dir}/CleanupDialog.h
        ${_labeler_dir}/CleanupDialog.cpp
        ${_labeler_dir}/commands/DiscardSliceCommand.h
        ${_labeler_dir}/commands/DiscardSliceCommand.cpp
        ${_labeler_dir}/pages/BuildCsvPage.h
        ${_labeler_dir}/pages/BuildCsvPage.cpp
        ${_labeler_dir}/pages/BuildDsPage.h
        ${_labeler_dir}/pages/BuildDsPage.cpp
        ${_labeler_dir}/pages/GameAlignPage.h
        ${_labeler_dir}/pages/GameAlignPage.cpp
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets minlabel-page phonemelabeler-page
                pitchlabeler-page pipeline-pages game-infer rmvpe-infer
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
)

# Include labeler directory so shared headers are found
target_include_directories(${PROJECT_NAME} PRIVATE
    ${_labeler_dir}
    ${_labeler_dir}/commands
    ${_labeler_dir}/pages
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
