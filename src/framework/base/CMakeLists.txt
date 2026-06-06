project(dsfw-base)

if (NOT TARGET dsfw::types)
    find_package(dsfw-types CONFIG REQUIRED)
endif ()

find_package(nlohmann_json CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        NAMESPACE dsfw::base
        EXPORT_SET dsfwTargets
        DEPENDS
        PUBLIC dsfw::types nlohmann_json::nlohmann_json
)
