project(chart-framework)

find_package(FFTW3 CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
        STATIC
        AUTOMOC
        DEPENDS
        PUBLIC dstools-domain FFTW3::fftw3
        PRIVATE Qt${QT_VERSION_MAJOR}::Widgets dsfw-ui-core
)