project(QsMedia)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE _src *.h *.cpp)

add_library(${PROJECT_NAME} STATIC ${_src})

target_compile_definitions(${PROJECT_NAME} PUBLIC QSMEDIA_STATIC)

target_include_directories(${PROJECT_NAME} PUBLIC .)

target_include_directories(${PROJECT_NAME} PRIVATE Api NAudio
    ${Qt${QT_VERSION_MAJOR}Core_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)