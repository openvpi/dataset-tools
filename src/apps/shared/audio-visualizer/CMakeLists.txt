cmake_minimum_required(VERSION 3.21)
project(audio-visualizer LANGUAGES CXX)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::shared::visualizer
        EXPORT_SET dstoolsSharedTargets
        DEPENDS
        PUBLIC dstools::shared::chart dsfw-core dsfw-ui-core dsfw-widgets dstools::shared::mouth-curve dstools::shared::phoneme
        PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
)
