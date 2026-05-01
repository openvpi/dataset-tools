project(dstools-domain)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(SndFile CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-core dstools-types Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui
               Qt${QT_VERSION_MAJOR}::Network nlohmann_json::nlohmann_json
        PRIVATE textgrid SndFile::sndfile
)
