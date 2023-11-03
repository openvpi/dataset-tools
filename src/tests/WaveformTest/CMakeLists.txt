project(WaveformTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)

add_executable(${PROJECT_NAME} ${_src}
        WaveformWidget.cpp
        WaveformWidget.h)

target_include_directories(${PROJECT_NAME} PUBLIC .)

find_package(SndFile CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        ${PLATFORM_LIB}
)