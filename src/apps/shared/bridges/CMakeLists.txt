project(shared-bridges)

dstools_add_library(${PROJECT_NAME}
        STATIC
        DEPENDS
        PUBLIC dstools-domain Qt${QT_VERSION_MAJOR}::Core
        INCLUDE_DIRS
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)