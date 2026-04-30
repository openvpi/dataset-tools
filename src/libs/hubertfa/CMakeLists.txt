project(hubertfa-lib)

set(CMAKE_AUTOMOC ON)

set(HUBERTFA_SOURCES
    HfaTask.cpp
)

set(HUBERTFA_HEADERS
    HfaTask.h
)

add_library(${PROJECT_NAME} STATIC ${HUBERTFA_SOURCES} ${HUBERTFA_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    hubert-infer
    Qt${QT_VERSION_MAJOR}::Core
)
