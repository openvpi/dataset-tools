project(lyricfa-lib)

find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-core dstools-infer-common audio-util FunAsr cpp-pinyin::cpp-pinyin
               SndFile::sndfile nlohmann_json::nlohmann_json
               Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
