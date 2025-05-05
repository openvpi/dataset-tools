project(SomeInfer VERSION 0.0.0.1)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src})

find_package(SndFile CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        some-infer::some-infer
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME} PRIVATE .)


if (WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)

    if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif ()

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

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})