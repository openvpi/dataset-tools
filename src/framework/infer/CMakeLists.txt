project(dstools-infer-common)

find_package(nlohmann_json CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    EXPORT_SET ${PROJECT_NAME}Targets
    DEPENDS
        PUBLIC dstools-types nlohmann_json::nlohmann_json onnxruntime
    INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../infer/onnxruntime/include>
)

target_link_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../infer/onnxruntime/lib>
)

# Package config for standalone consumption
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

configure_package_config_file(
    "${PROJECT_CMAKE_MODULES_DIR}/dstools-infer-commonConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/dstools-infer-commonConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/dstools-infer-commonConfig.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
)

if(ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_DML)
endif()

if(ONNXRUNTIME_ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_CUDA)
endif()
