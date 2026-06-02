set(CMAKE_FOLDER "Applications/Shared")
add_subdirectory(shared)
add_subdirectory(unified-editor)

set(CMAKE_FOLDER "Applications")
add_subdirectory(label-suite)
add_subdirectory(ds-labeler)
add_subdirectory(cli)
add_subdirectory(widget-gallery)
