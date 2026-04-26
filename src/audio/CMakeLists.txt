project(dstools-audio)

set(CMAKE_AUTOMOC ON)

set(AUDIO_SOURCES
    src/WaveFormat.cpp
    src/AudioDecoder.cpp
    src/AudioPlayback.cpp
)

set(AUDIO_HEADERS
    include/dstools/WaveFormat.h
    include/dstools/AudioDecoder.h
    include/dstools/AudioPlayback.h
)

add_library(${PROJECT_NAME} STATIC ${AUDIO_SOURCES} ${AUDIO_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Internal: access FFmpegDecoder and SDLPlayback directly
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/ffmpegdecoder
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/sdlplayback
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/qsmedia
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-core
    Qt${QT_VERSION_MAJOR}::Core
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    QsMedia
    FFmpegDecoder
    SDLPlayback
)
