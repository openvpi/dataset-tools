project(dsfw-base)

# Standalone build guards — only activate when targets are not already defined
if(NOT TARGET dstools-types::dstools-types)
    find_package(dstools-types CONFIG REQUIRED)
endif()

find_package(nlohmann_json CONFIG REQUIRED)

set(DSFW_BASE_SOURCES
    src/JsonHelper.cpp
)

set(DSFW_BASE_HEADERS
    include/dsfw/JsonHelper.h
)

add_library(${PROJECT_NAME} STATIC ${DSFW_BASE_SOURCES} ${DSFW_BASE_HEADERS})
add_library(dsfw::base ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-types::dstools-types
    nlohmann_json::nlohmann_json
)

# Install rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT dsfwTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY include/
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
