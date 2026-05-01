project(gameinfer-lib)

set(CMAKE_AUTOMOC ON)

set(GAMEINFER_SOURCES
    GameInferService.cpp
    GameMidiProcessor.cpp
)

set(GAMEINFER_HEADERS
    GameInferService.h
    GameMidiProcessor.h
)

add_library(${PROJECT_NAME} STATIC ${GAMEINFER_SOURCES} ${GAMEINFER_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    dstools-infer-common
    game-infer
    Qt${QT_VERSION_MAJOR}::Core
)
