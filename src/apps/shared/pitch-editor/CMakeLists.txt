project(pitch-editor)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::shared::pitch
        EXPORT_SET dstoolsSharedTargets
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/commands>
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../audio-visualizer>
        DEPENDS
        PUBLIC dstools::shared::chart dstools-domain
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
        PRIVATE dstools::shared::visualizer dstools::shared::settings
)

dstools_add_translations(${PROJECT_NAME})
