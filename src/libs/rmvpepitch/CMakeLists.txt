project(rmvpepitch-lib)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-core rmvpe-infer Qt${QT_VERSION_MAJOR}::Core
)
