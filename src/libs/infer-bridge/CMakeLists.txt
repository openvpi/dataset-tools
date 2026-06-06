project(infer-bridge)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        DEPENDS
        PUBLIC dsfw-core dsfw-infer hubert-infer rmvpe-infer game-infer lyricfa-lib
        Qt${QT_VERSION_MAJOR}::Core
        PRIVATE dstools-domain
)