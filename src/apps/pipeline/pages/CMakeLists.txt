project(pipeline-pages)

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE
    pipeline-slicer-page
    pipeline-lyricfa-page
    pipeline-hubertfa-page
)
