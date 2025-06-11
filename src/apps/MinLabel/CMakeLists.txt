project(MinLabel VERSION 0.0.1.8)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)
add_executable(${PROJECT_NAME} ${_src} res.qrc)

find_package(cpp-pinyin CONFIG REQUIRED)
find_package(cpp-kana CONFIG REQUIRED)
find_package(qBreakpad CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        cpp-pinyin::cpp-pinyin
        cpp-kana::cpp-kana
        SDLPlayback FFmpegDecoder
        qBreakpad::qBreakpad
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
endif ()

if (APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif ()

set(DIRECTORIES_TO_COPY
)

foreach (dir ${DIRECTORIES_TO_COPY})
    if (APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND
                ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/res/${dir}
                $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/${dir}
        )
    else ()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND
                ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/res/${dir}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/${dir}
        )

        install(
                DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/${dir}
                DESTINATION .
        )
    endif ()
endforeach ()

if (WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
            ${CMAKE_BINARY_DIR}/bin/dict
    )

    install(DIRECTORY ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cpp-pinyin/dict
            DESTINATION .
    )
endif ()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})