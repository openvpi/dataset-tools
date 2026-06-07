project(dsfw-widgets)

dstools_add_library(${PROJECT_NAME}
        SHARED
        AUTOMOC
        NAMESPACE dsfw::widgets
        EXPORT_SET dsfwTargets
        DEPENDS
        PUBLIC dsfw-core dsfw-audio-playback Qt${QT_VERSION_MAJOR}::Widgets
        PRIVATE dsfw-ui-core
        DEFINITIONS
        PRIVATE DSFW_WIDGETS_BUILD
)

dstools_add_translations(${PROJECT_NAME})
