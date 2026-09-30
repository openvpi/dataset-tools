project(Tifa VERSION 0.0.0.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.hpp *.cpp)
add_executable(${PROJECT_NAME} ${_src})

find_package(SndFile CONFIG REQUIRED)
find_package(cpp-pinyin CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

include_directories(../../libs/onnxruntime/include)
target_include_directories(${PROJECT_NAME} PUBLIC ../../libs/onnxruntime/include)
target_link_directories(${PROJECT_NAME} PUBLIC ../../libs/onnxruntime/lib)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        audio-util
        cpp-pinyin::cpp-pinyin
        nlohmann_json::nlohmann_json
        onnxruntime
        AsyncTaskWindow::AsyncTaskWindow
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

    # dxgi enumerates the adapters offered by the DirectML provider.
    if ("$ENV{WindowsSDKVersion}" MATCHES [[^([0-9.]*)(\\|/)?$]])
        set(WINDOWS_SDK_VERSION "${CMAKE_MATCH_1}")
    else ()
        message(WARNING "Failed to parse Windows SDK version from ENV{WindowsSDKVersion}.")
    endif ()

    set(WINDOWS_SDK_DIR "$ENV{WindowsSdkDir}")

    if (WINDOWS_SDK_DIR AND WINDOWS_SDK_VERSION)
        include_directories("${WINDOWS_SDK_DIR}Include/${WINDOWS_SDK_VERSION}/um")
        link_directories("${WINDOWS_SDK_DIR}Lib/${WINDOWS_SDK_VERSION}/um/x64")

        target_link_libraries(${PROJECT_NAME} PRIVATE dxgi)
    else ()
        message(WARNING "Windows SDK not found. Please ensure Windows SDK is installed.")
    endif ()
endif ()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif ()

# The Mandarin and Cantonese dictionaries of cpp-pinyin are taken from the
# vcpkg share directory, next to the dictionaries of TIFA. Builds that get
# cpp-pinyin from somewhere else have to copy them by hand.
if (WIN32)
    set(TIFA_CPP_PINYIN_DICT "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict")
    if (EXISTS "${TIFA_CPP_PINYIN_DICT}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${TIFA_CPP_PINYIN_DICT}"
                ${CMAKE_BINARY_DIR}/bin/dict
        )

        install(DIRECTORY "${TIFA_CPP_PINYIN_DICT}"
                DESTINATION .
        )
    else ()
        message(WARNING "Tifa: ${TIFA_CPP_PINYIN_DICT} does not exist; copy the mandarin and cantonese dictionaries of cpp-pinyin into ${CMAKE_BINARY_DIR}/bin/dict yourself.")
    endif ()
endif ()

# Dictionaries of TIFA; see dict/README.md for how to obtain them.
set(TIFA_DICT_FILES
        ds-zh-pinyin-lite.txt
        jyutping_dict.txt
        japanese_dict_full.txt
        ds_cmudict-07b.txt
)

foreach (dict_file ${TIFA_DICT_FILES})
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dict/${dict_file})
        if (APPLE)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_SOURCE_DIR}/dict/${dict_file}
                    $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/dict/${dict_file}
            )
        else ()
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${CMAKE_CURRENT_SOURCE_DIR}/dict/${dict_file}
                    ${CMAKE_BINARY_DIR}/bin/dict/${dict_file}
            )

            install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dict/${dict_file}
                    DESTINATION dict
            )
        endif ()
    else ()
        message(WARNING "Tifa: dict/${dict_file} is missing and will not be deployed.")
    endif ()
endforeach ()

# Optional English OOV model (LstmG2p-Eng); see dict/README.md.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/dict/LstmG2p-Eng)
    if (APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/dict/LstmG2p-Eng
                $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/dict/LstmG2p-Eng
        )
    else ()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/dict/LstmG2p-Eng
                ${CMAKE_BINARY_DIR}/bin/dict/LstmG2p-Eng
        )

        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dict/LstmG2p-Eng
                DESTINATION dict
        )
    endif ()
endif ()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})
