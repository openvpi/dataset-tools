project(dstools-infer-common)

set(INFER_COMMON_SOURCES
    src/OnnxEnv.cpp
)

set(INFER_COMMON_HEADERS
    include/dstools/ExecutionProvider.h
    include/dstools/OnnxEnv.h
)

add_library(${PROJECT_NAME} STATIC ${INFER_COMMON_SOURCES} ${INFER_COMMON_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    ${CMAKE_CURRENT_SOURCE_DIR}/../onnxruntime/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../onnxruntime/lib
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    onnxruntime
    Qt${QT_VERSION_MAJOR}::Core
)

if(ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_DML)
endif()

if(ONNXRUNTIME_ENABLE_CUDA)
    target_compile_definitions(${PROJECT_NAME} PUBLIC ONNXRUNTIME_ENABLE_CUDA)
endif()
