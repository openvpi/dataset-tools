project(dstools-infer-common)

find_package(nlohmann_json CONFIG REQUIRED)

if(NOT TARGET onnxruntime)
    add_library(onnxruntime INTERFACE IMPORTED)
    set(_ORT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../infer/onnxruntime")
    target_include_directories(onnxruntime INTERFACE
        $<BUILD_INTERFACE:${_ORT_ROOT}/include>
    )
    if(WIN32)
        set(_ORT_LINK_LIB "${_ORT_ROOT}/lib/onnxruntime.lib")
    elseif(APPLE)
        set(_ORT_LINK_LIB "${_ORT_ROOT}/lib/libonnxruntime.dylib")
    else()
        set(_ORT_LINK_LIB "${_ORT_ROOT}/lib/libonnxruntime.so")
    endif()
    target_link_libraries(onnxruntime INTERFACE
        $<BUILD_INTERFACE:${_ORT_LINK_LIB}>
    )
endif()

dstools_add_library(${PROJECT_NAME}
    STATIC
    EXPORT_SET ${PROJECT_NAME}Targets
    DEPENDS
        PUBLIC dstools-types nlohmann_json::nlohmann_json onnxruntime
)

# Package config for standalone consumption
if(NOT DSTOOLS_DEPLOY_MODE)
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
endif()

if(MSVC)
    # C4275: non dll-interface class used as base for dll-interface class.
    # This is expected because dstools-infer-common is a static library whose
    # classes are inherited by DLL-exported classes in consumer libraries
    # (hubert-infer, game-infer, rmvpe-infer).  The static lib is linked
    # directly into each DLL, so the warning is benign.
    target_compile_options(${PROJECT_NAME} PUBLIC /wd4275)
endif()

if(ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_DML)
endif()

if(ONNXRUNTIME_ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_CUDA)
endif()
