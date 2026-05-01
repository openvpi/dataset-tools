project(dsfw-widgets-test)

enable_testing()

add_executable(${PROJECT_NAME} TestDsfwWidgets.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dsfw-widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
