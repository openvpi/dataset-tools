project(dstools-widgets)

dstools_add_library(${PROJECT_NAME}
    SHARED
    AUTOMOC
    NAMESPACE ${PROJECT_NAME}::${PROJECT_NAME}
    DEPENDS
        PUBLIC dsfw-widgets Qt${QT_VERSION_MAJOR}::Widgets
        PRIVATE $<$<PLATFORM_ID:Windows>:dxgi>
    DEFINITIONS
        PRIVATE DSTOOLS_WIDGETS_BUILD
)

dstools_add_translations(${PROJECT_NAME})
