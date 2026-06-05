project(data-sources)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        DEPENDS
        PUBLIC dstools-domain dsfw-core dsfw-signal dsfw-widgets settings-page shared-bridges
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        PRIVATE moelib
        infer-bridge lyricfa-lib log-page
        PUBLIC min-label-editor phoneme-editor
        pitch-editor audio-visualizer
)
