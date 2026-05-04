project(data-sources)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)
find_package(SndFile REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dstools-domain dsfw-core dsfw-widgets settings-page
               min-label-editor phoneme-editor pitch-editor
               lyricfa-lib hubertfa-lib rmvpepitch-lib gameinfer-lib
               audio-util
               SndFile::sndfile
               Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
               Qt${QT_VERSION_MAJOR}::Concurrent
)
