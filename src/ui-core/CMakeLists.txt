project(dstools-ui-core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(UI_CORE_SOURCES
    src/Theme.cpp
    src/FramelessHelper.cpp
    src/AppInit.cpp
)

set(UI_CORE_HEADERS
    include/dstools/Theme.h
    include/dstools/FramelessHelper.h
    include/dstools/AppInit.h
    include/dstools/IStepPlugin.h
)

set(UI_CORE_RESOURCES
    res/ui-core.qrc
)

add_library(${PROJECT_NAME} STATIC ${UI_CORE_SOURCES} ${UI_CORE_HEADERS} ${UI_CORE_RESOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    dstools-core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# QWindowKit for frameless windows
find_package(QWindowKit CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC QWindowKit::Widgets)

# Optional: qBreakpad support (used by AppInit)
find_package(qBreakpad CONFIG QUIET)
if(qBreakpad_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE qBreakpad::qBreakpad)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_QBREAKPAD)
endif()

# Optional: cpp-pinyin support (used by AppInit)
find_package(cpp-pinyin CONFIG QUIET)
if(cpp-pinyin_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE cpp-pinyin::cpp-pinyin)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_CPP_PINYIN)
endif()
