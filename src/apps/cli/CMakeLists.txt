project(dstools-cli)

find_package(syscmdline REQUIRED)
find_package(SndFile CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
        SOURCES main.cpp
        DEPENDS
        PRIVATE dsfw-core dstools-domain slicer-lib lyricfa-lib hubertfa-lib
        infer-bridge audio-util
        syscmdline::syscmdline SndFile::sndfile Qt${QT_VERSION_MAJOR}::Core
)
