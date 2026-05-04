cmake_minimum_required(VERSION 3.21)
project(audio-visualizer LANGUAGES CXX)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    DEPENDS
        PUBLIC dsfw-core dsfw-widgets phoneme-editor
        PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
)
