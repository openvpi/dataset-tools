project(hfa-cli)

find_package(SndFile CONFIG REQUIRED)

dstools_add_executable(${PROJECT_NAME}
    SOURCES main.cpp
    DEPENDS
        PRIVATE hubert-infer::hubert-infer
                audio-util::audio-util
                SndFile::sndfile
                Qt${QT_VERSION_MAJOR}::Core
)
