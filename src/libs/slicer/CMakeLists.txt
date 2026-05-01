project(slicer-lib)

find_package(SndFile CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    DEPENDS
        PUBLIC dsfw-core audio-util Qt${QT_VERSION_MAJOR}::Core SndFile::sndfile
)
