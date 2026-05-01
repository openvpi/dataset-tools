project(pipeline-hubertfa-page)

set(CMAKE_AUTOMOC ON)

set(HUBERTFA_PAGE_SOURCES
    HubertFAPage.cpp
)

set(HUBERTFA_PAGE_HEADERS
    HubertFAPage.h
)

add_library(${PROJECT_NAME} STATIC ${HUBERTFA_PAGE_SOURCES} ${HUBERTFA_PAGE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-ui-core
    dstools-widgets
    dstools-domain
    hubertfa-lib
    hubert-infer
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)
