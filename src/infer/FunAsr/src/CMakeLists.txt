set(FUNASR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FeatureExtract.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FeatureQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/paraformer_onnx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SpeechWrap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Vocab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/alignedmem.cpp
)

add_library(${PROJECT_NAME} ${FUNASR_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../onnxruntime/include
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../onnxruntime/lib
)

find_package(fftw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC fftw3 fftw3f)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -D_RPASR_API_EXPORT)
else()
    find_package(OpenBLAS CONFIG QUIET)
    if (OpenBLAS_FOUND)
        target_link_libraries(${PROJECT_NAME} PUBLIC OpenBLAS::OpenBLAS)
    endif()
    set(EXTRA_LIBS pthread)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC onnxruntime ${EXTRA_LIBS})
