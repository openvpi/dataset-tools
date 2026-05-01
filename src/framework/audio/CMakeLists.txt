project(dstools-audio)

find_package(FFmpeg CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    EXPORT_SET dsfwTargets
    DEPENDS
        PUBLIC Qt${QT_VERSION_MAJOR}::Core
        PRIVATE SDL2::SDL2
)

# Link FFmpeg libraries
ffmpeg_link_all(${PROJECT_NAME} PRIVATE)
