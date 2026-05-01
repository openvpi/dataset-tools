project(dsfw-widgets)

dstools_add_library(${PROJECT_NAME}
    SHARED
    AUTOMOC
    NAMESPACE dsfw::widgets
    EXPORT_SET dsfwTargets
    DEPENDS
        PUBLIC dsfw-core Qt${QT_VERSION_MAJOR}::Widgets
        PRIVATE dsfw-ui-core dstools-audio
    DEFINITIONS
        PRIVATE DSFW_WIDGETS_BUILD
)
