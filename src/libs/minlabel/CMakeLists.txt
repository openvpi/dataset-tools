project(minlabel-lib)

set(MINLABEL_SOURCES
    MinLabelService.cpp
)

set(MINLABEL_HEADERS
    MinLabelService.h
)

add_library(${PROJECT_NAME} STATIC ${MINLABEL_SOURCES} ${MINLABEL_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    dstools-domain
    Qt${QT_VERSION_MAJOR}::Core
)
