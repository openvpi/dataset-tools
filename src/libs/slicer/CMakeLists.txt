project(slicer-lib)

dstools_add_library(${PROJECT_NAME}
        STATIC
        DEPENDS
        PUBLIC dsfw-core dsfw-audio dsfw-signal Qt${QT_VERSION_MAJOR}::Core
)
