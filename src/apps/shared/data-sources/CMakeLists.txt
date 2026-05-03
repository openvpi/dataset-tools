project(data-sources)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dstools-domain dsfw-core Qt${QT_VERSION_MAJOR}::Core
)
