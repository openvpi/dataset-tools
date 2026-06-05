project(dsfw-audio)

find_package(FFmpeg REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    EXPORT_SET dsfwTargets
    DEPENDS
    PUBLIC dstools-types Qt${QT_VERSION_MAJOR}::Core
    PRIVATE dsfw-core SDL2::SDL2
)

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})

add_subdirectory(tests)