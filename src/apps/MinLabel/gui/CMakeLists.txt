project(minlabel-page)

find_package(cpp-pinyin CONFIG REQUIRED)
find_package(cpp-kana CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dstools-widgets dstools-domain dsfw-ui-core minlabel-lib
               Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
               cpp-pinyin::cpp-pinyin cpp-kana::cpp-kana
)
