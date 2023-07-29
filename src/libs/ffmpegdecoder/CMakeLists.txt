project(FFmpegDecoder)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)

add_library(${PROJECT_NAME} STATIC ${_src})

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_link_libraries(${PROJECT_NAME} PUBLIC
    QsMedia
)

find_package(ffmpeg-fake REQUIRED NAMES FFmpeg)

ffmpeg_link_libraries(${PROJECT_NAME} PRIVATE avcodec avformat avutil swresample)
