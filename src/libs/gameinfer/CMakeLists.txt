project(gameinfer-lib)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-core dstools-infer-common game-infer Qt${QT_VERSION_MAJOR}::Core
)
