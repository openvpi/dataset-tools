project(slicer-lib)

set(SLICER_SOURCES
    Slicer.cpp
    SlicerService.cpp
)

set(SLICER_HEADERS
    Slicer.h
    SlicerService.h
    MathUtils.h
    Enumerations.h
)

add_library(${PROJECT_NAME} STATIC ${SLICER_SOURCES} ${SLICER_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

find_package(SndFile CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
    SndFile::sndfile
)
