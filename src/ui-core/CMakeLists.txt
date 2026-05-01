project(dstools-ui-core)

set(CMAKE_AUTOMOC ON)

set(UI_CORE_SOURCES
    src/AppInit.cpp
)

set(UI_CORE_HEADERS
    include/dstools/AppInit.h
)

add_library(${PROJECT_NAME} STATIC ${UI_CORE_SOURCES} ${UI_CORE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-ui-core
    dsfw-core
    dstools-domain
    Qt${QT_VERSION_MAJOR}::Widgets
)
