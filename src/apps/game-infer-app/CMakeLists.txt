project(GameInfer VERSION 0.1.0.0)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC AUTOUIC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    DEPENDS
        PRIVATE dstools-ui-core dstools-widgets dsfw-ui-core game-infer::game-infer
                Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
