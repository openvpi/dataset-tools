project(dstools-core)

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE
    dstools-domain
    dsfw-core
)
