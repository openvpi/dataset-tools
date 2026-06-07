project(dsfw-audio)

find_package(FFmpeg REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# Core audio library: no Qt dependency
dstools_add_library(${PROJECT_NAME}
    STATIC
    NAMESPACE dsfw::audio
    EXPORT_SET dsfwTargets
    DEPENDS
    PUBLIC dsfw::types
    PRIVATE dsfw-core SDL2::SDL2
)

target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})

add_subdirectory(playback)
add_subdirectory(tests)