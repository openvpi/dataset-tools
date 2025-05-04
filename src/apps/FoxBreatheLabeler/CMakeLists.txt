project(FoxBreatheLabeler VERSION 0.0.0.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.hpp *.cpp)
add_executable(${PROJECT_NAME} ${_src} res/qss.qrc)

find_package(SndFile CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

include_directories(../../libs/onnxruntime/include)
target_include_directories(${PROJECT_NAME} PUBLIC ../../libs/onnxruntime/include)
target_link_directories(${PROJECT_NAME} PUBLIC ../../libs/onnxruntime/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        QMCore
        audio-util yaml-cpp::yaml-cpp
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

if (WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)

    if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()
endif ()

set(DIRECTORIES_TO_COPY
        dict
)

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)

    foreach (dir ${DIRECTORIES_TO_COPY})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND
                ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_BINARY_DIR}/bin/${dir}
                $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/${dir}
        )
    endforeach ()
endif ()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})