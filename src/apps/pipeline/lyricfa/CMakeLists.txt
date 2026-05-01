project(pipeline-lyricfa-page)

find_package(cpp-pinyin CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-ui-core dstools-widgets lyricfa-lib dsfw-core
               cpp-pinyin::cpp-pinyin Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
