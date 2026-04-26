project(hfa-cli)

find_package(SndFile CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
        SOURCES main.cpp
        DEPENDS
        PUBLIC audio-util Qt${QT_VERSION_MAJOR}::Core
        PRIVATE infer-bridge SndFile::sndfile
)
