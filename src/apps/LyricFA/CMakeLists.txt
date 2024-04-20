project(LyricFA VERSION 0.0.0.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src})

find_package(SndFile CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        CkIKg2p
        QMCore
        r8brain
        rapidasr
)
target_include_directories(${PROJECT_NAME} PRIVATE ../../libs/RapidASR/cpp_onnx/include)

file(GLOB_RECURSE onnx_files ../../libs/onnxruntime/lib/*.*)
foreach (onnx_file ${onnx_files})
    file(COPY ${onnx_file} DESTINATION ${CMAKE_BINARY_DIR}/bin)
endforeach ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

if (WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)

    if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()
endif ()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif ()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})