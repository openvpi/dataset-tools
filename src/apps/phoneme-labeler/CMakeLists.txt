project(PhonemeLabeler VERSION 0.1.0.0)

add_subdirectory(gui)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC AUTOUIC
    DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
    SOURCES main.cpp PhonemeLabelerKeys.h
    DEPENDS
        PRIVATE dstools-ui-core phonemelabeler-page
)
