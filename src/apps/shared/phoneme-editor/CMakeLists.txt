project(phoneme-editor)

find_package(FFTW3 CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui/commands>
    DEPENDS
        PUBLIC dstools-widgets dstools-domain dstools-audio dsfw-ui-core
               textgrid FFTW3::fftw3
               Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)

dstools_add_translations(${PROJECT_NAME})
