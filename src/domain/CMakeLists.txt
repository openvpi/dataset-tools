project(dstools-domain)

set(CMAKE_AUTOMOC ON)

set(DOMAIN_SOURCES)
set(DOMAIN_HEADERS)

add_library(${PROJECT_NAME} STATIC ${DOMAIN_SOURCES} ${DOMAIN_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)
