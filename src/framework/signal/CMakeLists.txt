project(dsfw-signal)

if (NOT TARGET dstools-types::dstools-types)
    find_package(dstools-types CONFIG REQUIRED)
endif ()

dstools_add_library(${PROJECT_NAME}
        STATIC
        NAMESPACE dsfw::signal
        EXPORT_SET dsfwTargets
        DEPENDS
        PUBLIC dstools-types::dstools-types
)