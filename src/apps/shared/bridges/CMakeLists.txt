project(shared-bridges)

dstools_add_library(${PROJECT_NAME}
        STATIC
        NAMESPACE dstools::shared::bridges
        EXPORT_SET dstoolsSharedTargets
        DEPENDS
        PUBLIC dstools-domain Qt${QT_VERSION_MAJOR}::Core
        INCLUDE_DIRS
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)