cmake_minimum_required(VERSION 3.21)
project(audio-visualizer LANGUAGES CXX)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        DEPENDS
        PUBLIC chart-framework dsfw-core dsfw-ui-core dsfw-widgets mouth-curve-chart phoneme-editor
        PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
)
