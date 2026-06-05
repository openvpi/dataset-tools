project(moe-infer VERSION 1.0.0.0 LANGUAGES CXX)

include(${PROJECT_CMAKE_MODULES_DIR}/infer-target.cmake)

dstools_add_infer_library(${PROJECT_NAME}
        STATIC_OPTION MOE_INFER_BUILD_STATIC
        TESTS_OPTION MOE_INFER_BUILD_TESTS
        INSTALL_OPTION MOE_INFER_INSTALL
        LIBRARY_DEFINE MOE_INFER_LIBRARY
        STATIC_DEFINE MOE_INFER_STATIC
        RC_DESCRIPTION "MOE mouth curve infer."
        RC_COPYRIGHT "Copyright (C) 2025"
)

target_link_libraries(${PROJECT_NAME} PUBLIC dsfw-core infer-common)