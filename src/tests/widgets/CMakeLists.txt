project(dsfw-widgets-test)

set(CMAKE_AUTOMOC ON)

enable_testing()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

add_executable(${PROJECT_NAME} TestDsfwWidgets.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
        dsfw-widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

add_executable(TestAudioVisualizerContainer TestAudioVisualizerContainer.cpp)

target_include_directories(TestAudioVisualizerContainer PRIVATE
        ${CMAKE_SOURCE_DIR}/src/apps/shared
)

target_link_libraries(TestAudioVisualizerContainer PRIVATE
        audio-visualizer
        dsfw-widgets
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME TestAudioVisualizerContainer COMMAND TestAudioVisualizerContainer)
