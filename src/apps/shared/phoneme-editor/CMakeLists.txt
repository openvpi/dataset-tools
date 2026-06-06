project(phoneme-editor)

find_package(FFTW3 CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        NAMESPACE dstools::shared::phoneme
        EXPORT_SET dstoolsSharedTargets
        INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/commands>
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../audio-visualizer>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../mouth-curve-chart>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../pitch-editor>
        DEPENDS
        PUBLIC dstools::shared::chart dstools-domain dsfw-audio dsfw-ui-core
        textgrid FFTW3::fftw3
        Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
        dstools::shared::pitch
)

dstools_add_translations(${PROJECT_NAME})
