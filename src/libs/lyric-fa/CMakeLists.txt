project(lyricfa-lib)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        DEPENDS
        PUBLIC dsfw-core dsfw-audio dsfw-signal cpp-pinyin::cpp-pinyin
        nlohmann_json::nlohmann_json
        Qt${QT_VERSION_MAJOR}::Core
        PRIVATE FunAsr
)
