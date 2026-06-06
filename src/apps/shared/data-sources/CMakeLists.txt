project(data-sources)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::shared::data-sources
        EXPORT_SET dstoolsSharedTargets
        DEPENDS
        PUBLIC dstools-domain dsfw-core dsfw-signal dsfw-widgets dstools::shared::settings dstools::shared::bridges
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        PRIVATE moelib
        infer-bridge lyricfa-lib dstools::shared::log
        PUBLIC dstools::shared::min-label dstools::shared::phoneme
        dstools::shared::pitch dstools::shared::visualizer
)
