project(minlabel-lib)

dstools_add_library(${PROJECT_NAME}
    STATIC
    DEPENDS
        PUBLIC dsfw-core dstools-domain Qt${QT_VERSION_MAJOR}::Core
)
