project(TestShell)

set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dstools-ui-core
    dstools-widgets
    dsfw-ui-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(WIN32 AND NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
