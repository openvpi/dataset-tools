project(TestShell)

dstools_add_executable(${PROJECT_NAME}
    AUTOMOC
    WIN32_EXECUTABLE
    SOURCES main.cpp
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets dsfw-ui-core
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
