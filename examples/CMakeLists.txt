option(BUILD_EXAMPLES "Build example applications" OFF)

if(BUILD_EXAMPLES)
    add_subdirectory(minimal-task-processor)
    add_subdirectory(minimal-appshell)
endif()
