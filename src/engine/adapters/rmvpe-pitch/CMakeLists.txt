project(rmvpepitch-lib)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::rmvpe-pitch
        DEPENDS
        PUBLIC dsfw-core Qt${QT_VERSION_MAJOR}::Core
        PRIVATE rmvpe-infer
)
