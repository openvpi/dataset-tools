project(dstools-cli)

find_package(syscmdline REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    SOURCES main.cpp
    DEPENDS
        PRIVATE dsfw-core dstools-domain slicer-lib lyricfa-lib hubertfa-lib
                syscmdline::syscmdline Qt${QT_VERSION_MAJOR}::Core
)
