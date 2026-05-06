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
        PRIVATE dstools-ui-core dstools-widgets
                settings-page data-sources log-page
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/pages
)

dstools_add_translations(${PROJECT_NAME})

# Copy cpp-pinyin dictionary (needed for MinLabel G2P)
find_package(cpp-pinyin CONFIG REQUIRED)
set(_CPP_PINYIN_DICT_DIR "${cpp-pinyin_DIR}/dict")
if(NOT EXISTS "${_CPP_PINYIN_DICT_DIR}")
    message(FATAL_ERROR "cpp-pinyin dict not found at: ${_CPP_PINYIN_DICT_DIR}")
endif()
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${_CPP_PINYIN_DICT_DIR}"
    "${CMAKE_BINARY_DIR}/bin/dict"
)
install(DIRECTORY "${_CPP_PINYIN_DICT_DIR}"
    DESTINATION .
)
