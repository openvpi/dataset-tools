project(dsfw-core)

if (NOT TARGET dstools-types::dstools-types)
    find_package(dstools-types CONFIG REQUIRED)
endif ()

if (NOT TARGET Qt6::Core)
    find_package(QT NAMES Qt6 COMPONENTS Core Network Widgets REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Widgets REQUIRED)
endif ()

find_package(nlohmann_json CONFIG REQUIRED)

if (NOT TARGET onnxruntime)
    add_library(onnxruntime INTERFACE IMPORTED)
    set(_ORT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../infer/onnxruntime")
    target_include_directories(onnxruntime INTERFACE
            $<BUILD_INTERFACE:${_ORT_ROOT}/include>
    )
    if (WIN32)
        set(_ORT_LINK_LIB "${_ORT_ROOT}/lib/onnxruntime.lib")
    elseif (APPLE)
        set(_ORT_LINK_LIB "${_ORT_ROOT}/lib/libonnxruntime.dylib")
    else ()
        set(_ORT_LINK_LIB "${_ORT_ROOT}/lib/libonnxruntime.so")
    endif ()
    target_link_libraries(onnxruntime INTERFACE
            $<BUILD_INTERFACE:${_ORT_LINK_LIB}>
    )
endif ()

configure_file(
        include/dsfw/Version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/dsfw/Version.h
        @ONLY
)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dsfw::core
        EXPORT_SET dsfwTargets
        DEPENDS
        PUBLIC dsfw-base dstools-types::dstools-types Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network nlohmann_json::nlohmann_json onnxruntime
        PRIVATE $<$<PLATFORM_ID:Windows>:dbghelp> Qt${QT_VERSION_MAJOR}::Widgets
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
)

dstools_add_translations(${PROJECT_NAME} EXPORT_SET dsfwTargets)

# Merged dstools-infer-common sources (explicit listing)
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../infer/src/OnnxEnv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../infer/src/OnnxModelBase.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../infer/include>
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /wd4275)
endif ()

if (ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_DML)
endif ()

if (ONNXRUNTIME_ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_CUDA)
endif ()

# Install generated Version.h
if (NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/dsfw/Version.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/dsfw"
    )
endif ()
