project(dstools-audio)

set(CMAKE_AUTOMOC ON)

set(AUDIO_SOURCES
    src/WaveFormat.cpp
    src/AudioDecoder.cpp
    src/AudioPlayback.cpp
    src/AudioPlayer.cpp
)

set(AUDIO_HEADERS
    include/dstools/WaveFormat.h
    include/dstools/AudioDecoder.h
    include/dstools/AudioPlayback.h
    include/dstools/IAudioPlayer.h
    include/dstools/AudioPlayer.h
)

add_library(${PROJECT_NAME} STATIC ${AUDIO_SOURCES} ${AUDIO_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# FFmpeg
find_package(FFmpeg CONFIG REQUIRED)

# SDL2
find_package(SDL2 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL2::SDL2
)

# Link FFmpeg libraries
ffmpeg_link_all(${PROJECT_NAME} PRIVATE)
