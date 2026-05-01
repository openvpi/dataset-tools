project(rmvpepitch-lib)

set(CMAKE_AUTOMOC ON)

set(RMVPEPITCH_SOURCES
    RmvpePitchProcessor.cpp
)

set(RMVPEPITCH_HEADERS
    RmvpePitchProcessor.h
)

add_library(${PROJECT_NAME} STATIC ${RMVPEPITCH_SOURCES} ${RMVPEPITCH_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    rmvpe-infer
    Qt${QT_VERSION_MAJOR}::Core
)
