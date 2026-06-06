project(gameinfer-lib)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::game-infer-lib
        DEPENDS
        PUBLIC dsfw-core Qt${QT_VERSION_MAJOR}::Core
        PRIVATE game-infer
)
