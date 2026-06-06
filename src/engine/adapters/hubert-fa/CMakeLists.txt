project(hubertfa-lib)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::hubert-fa
        DEPENDS
        PUBLIC dsfw-core textgrid Qt${QT_VERSION_MAJOR}::Core
        PRIVATE hubert-infer
)
