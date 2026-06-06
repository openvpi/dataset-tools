project(settings-page)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::shared::settings
        EXPORT_SET dstoolsSharedTargets
        DEPENDS
        PUBLIC dstools-ui-core dsfw-widgets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
        PRIVATE dstools-domain infer-bridge
)
