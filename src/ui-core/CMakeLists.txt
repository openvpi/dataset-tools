project(dstools-ui-core)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-ui-core dsfw-core dstools-domain Qt${QT_VERSION_MAJOR}::Widgets
)
