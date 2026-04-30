add_subdirectory(common)

add_subdirectory(audio-util)
add_subdirectory(FunAsr)
add_subdirectory(rmvpe-infer)
add_subdirectory(game-infer)
add_subdirectory(hubert-infer)

# Copy ONNX Runtime (and DirectML) DLLs to output directory for development builds
set(_ORT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/onnxruntime/lib)
file(GLOB _ORT_DLLS "${_ORT_LIB_DIR}/*.dll")
if (_ORT_DLLS)
    file(COPY ${_ORT_DLLS} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif ()
