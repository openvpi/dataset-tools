project(dsfw-signal)

dstools_add_library(${PROJECT_NAME}
    STATIC
    NAMESPACE dsfw::signal
    EXPORT_SET dsfwTargets
    DEPENDS
    PUBLIC dsfw-core dsfw::types
)

# Optional: xsimd for vectorized calculations
find_package(xsimd CONFIG)
if (xsimd_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE xsimd)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DSFW_SIGNAL_ENABLE_XSIMD)
endif ()