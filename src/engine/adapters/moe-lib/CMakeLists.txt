project(moelib)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::moe-lib
        DEPENDS
        PUBLIC dstools-domain dsfw-audio moe-infer dsfw-infer
        PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Concurrent
)