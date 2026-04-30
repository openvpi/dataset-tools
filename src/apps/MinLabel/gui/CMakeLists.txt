project(minlabel-page)

set(CMAKE_AUTOMOC ON)

set(PAGE_SOURCES
    MinLabelPage.cpp
    MinLabelPage.h
    TextWidget.cpp
    TextWidget.h
    Common.cpp
    Common.h
    ExportDialog.cpp
    ExportDialog.h
)

add_library(${PROJECT_NAME} STATIC ${PAGE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(cpp-pinyin CONFIG REQUIRED)
find_package(cpp-kana CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-widgets
    dstools-domain
    labeler-interfaces
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    cpp-pinyin::cpp-pinyin
    cpp-kana::cpp-kana
)
