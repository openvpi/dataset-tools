project(MinLabel VERSION 0.0.1.7)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src} res.qrc)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    CkIKg2p
    QMCore SDLPlayback FFmpegDecoder
    qastool::core
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

include(qasc_exec.cmake)

if(WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)

    if(NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/dict
        $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/dict
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res/dict
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/dict
    )

    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/dict
        DESTINATION .
    )
endif()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})