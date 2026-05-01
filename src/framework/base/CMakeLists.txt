project(dsfw-base)

# Standalone build guards — only activate when targets are not already defined
if(NOT TARGET dstools-types::dstools-types)
    find_package(dstools-types CONFIG REQUIRED)
endif()

find_package(nlohmann_json CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    NAMESPACE dsfw::base
    EXPORT_SET dsfwTargets
    DEPENDS
        PUBLIC dstools-types::dstools-types nlohmann_json::nlohmann_json
)
