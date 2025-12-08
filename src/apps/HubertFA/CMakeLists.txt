project(HubertFA VERSION 0.0.0.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.hpp *.cpp)
add_executable(${PROJECT_NAME} ${_src})

find_package(SndFile CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

include_directories(../../libs/onnxruntime/include)
target_include_directories(${PROJECT_NAME} PUBLIC ../../libs/onnxruntime/include)
target_link_directories(${PROJECT_NAME} PUBLIC ../../libs/onnxruntime/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        audio-util nlohmann_json::nlohmann_json
        onnxruntime
)

file(GLOB_RECURSE onnx_files ../../libs/onnxruntime/lib/*.*)
foreach (onnx_file ${onnx_files})
    file(COPY ${onnx_file} DESTINATION ${CMAKE_BINARY_DIR}/bin)
endforeach ()

target_compile_definitions(${PROJECT_NAME} PRIVATE
        APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

if (ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ONNXRUNTIME_ENABLE_DML)
endif ()

if (WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)

    if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()
endif ()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})