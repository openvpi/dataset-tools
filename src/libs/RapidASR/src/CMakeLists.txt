file(GLOB_RECURSE _src *.h *.cpp)

add_library(rapidasr ${_src})
target_include_directories(rapidasr PUBLIC *.h)

find_package(fftw3 CONFIG REQUIRED)
target_include_directories(rapidasr PUBLIC ${FFTW3_INCLUDE_DIRS})
target_link_directories(rapidasr PUBLIC ${FFTW3_LIBRARY_DIRS})
target_link_libraries(rapidasr PUBLIC fftw3 fftw3f)

include_directories(../include)
target_include_directories(rapidasr PUBLIC ../../onnxruntime/include)
target_link_directories(rapidasr PUBLIC ../../onnxruntime/lib)

if (WIN32)
    target_compile_definitions(rapidasr PUBLIC -D_RPASR_API_EXPORT)
else ()
    set(EXTRA_LIBS pthread)
    target_include_directories(rapidasr PUBLIC "/usr/local/opt/fftw/include")
    target_link_directories(rapidasr PUBLIC "/usr/local/opt/fftw/lib")

    target_include_directories(rapidasr PUBLIC "/usr/local/opt/openblas/include")
    target_link_directories(rapidasr PUBLIC "/usr/local/opt/openblas/lib")

    target_include_directories(rapidasr PUBLIC "/usr/include")
    target_link_directories(rapidasr PUBLIC "/usr/lib64")
    include_directories(${ONNXRUNTIME_DIR}/include)
endif ()

target_include_directories(rapidasr PUBLIC ../include)
target_link_libraries(rapidasr PUBLIC onnxruntime ${EXTRA_LIBS})



