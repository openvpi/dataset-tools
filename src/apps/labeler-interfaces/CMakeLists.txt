project(labeler-interfaces)

# Thin compatibility shim — interfaces now live in dsfw-ui-core.
add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE dsfw-ui-core)
