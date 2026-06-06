project(mouth-curve-chart)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::shared::mouth-curve
        EXPORT_SET dstoolsSharedTargets
        DEPENDS
        PUBLIC dstools-domain dstools::shared::chart
        PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)