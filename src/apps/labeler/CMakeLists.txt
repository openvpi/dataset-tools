project(DiffSingerLabeler VERSION 0.1.0.0)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets minlabel-page phonemelabeler-page
                pitchlabeler-page pipeline-pages game-infer rmvpe-infer
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent
)
