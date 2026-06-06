project(rmvpe-infer VERSION 1.0.0.0 LANGUAGES CXX)

include(${PROJECT_CMAKE_MODULES_DIR}/infer-target.cmake)

dstools_add_infer_library(${PROJECT_NAME}
        STATIC_OPTION RMVPE_INFER_BUILD_STATIC
        TESTS_OPTION RMVPE_INFER_BUILD_TESTS
        INSTALL_OPTION RMVPE_INFER_INSTALL
        LIBRARY_DEFINE RMVPE_INFER_LIBRARY
        STATIC_DEFINE RMVPE_INFER_STATIC
        RC_DESCRIPTION "Rmvpe model infer."
        RC_COPYRIGHT "Copyright (C) 2023-2024 wolfgitpr"
)

# Library-specific dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC dsfw-audio dsfw-core dsfw-infer)
