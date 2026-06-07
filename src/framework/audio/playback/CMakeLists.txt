project(dsfw-audio-playback)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    NAMESPACE dsfw::audio::playback
    EXPORT_SET dsfwTargets
    DEPENDS
    PUBLIC dsfw-audio dsfw::types Qt${QT_VERSION_MAJOR}::Core
    PRIVATE dsfw-core
)