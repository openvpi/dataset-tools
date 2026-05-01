project(dstools-cli)

add_executable(${PROJECT_NAME} main.cpp)

find_package(syscmdline REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dsfw-core
    dstools-domain
    slicer-lib
    lyricfa-lib
    hubertfa-lib
    syscmdline::syscmdline
    Qt${QT_VERSION_MAJOR}::Core
)
