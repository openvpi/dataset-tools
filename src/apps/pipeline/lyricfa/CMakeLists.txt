project(pipeline-lyricfa-page)

set(CMAKE_AUTOMOC ON)

set(LYRICFA_PAGE_SOURCES
    LyricFAPage.cpp
)

set(LYRICFA_PAGE_HEADERS
    LyricFAPage.h
)

add_library(${PROJECT_NAME} STATIC ${LYRICFA_PAGE_SOURCES} ${LYRICFA_PAGE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(cpp-pinyin CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-widgets
    lyricfa-lib
    dsfw-core
    cpp-pinyin::cpp-pinyin
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
