project(pipeline-hubertfa-page)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-ui-core dstools-widgets dstools-domain hubertfa-lib hubert-infer dsfw-core
               Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
