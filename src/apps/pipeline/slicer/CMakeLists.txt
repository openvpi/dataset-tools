project(pipeline-slicer-page)

find_package(SndFile CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-ui-core dstools-widgets dstools-audio slicer-lib
               SndFile::sndfile Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
