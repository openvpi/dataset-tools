project(WidgetGallery)

dstools_add_executable(${PROJECT_NAME}
    SOURCES main.cpp
    DEPENDS
        PRIVATE dsfw-widgets Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
