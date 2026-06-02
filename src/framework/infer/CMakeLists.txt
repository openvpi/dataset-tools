project(infer-common)

if (NOT TARGET dstools-types::dstools-types)
    find_package(dstools-types CONFIG REQUIRED)
endif ()

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

if (NOT TARGET Qt6::Core)
    find_package(QT NAMES Qt6 COMPONENTS Core REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
endif ()

find_package(nlohmann_json CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        EXPORT_SET dsfwTargets
        DEPENDS
        PUBLIC dstools-types::dstools-types onnxruntime nlohmann_json::nlohmann_json Qt${QT_VERSION_MAJOR}::Core
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../core/include>
)