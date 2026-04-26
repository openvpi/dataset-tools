project(dstools-core)

set(CMAKE_AUTOMOC ON)

set(CORE_SOURCES
    src/AppInit.cpp
    src/Config.cpp
    src/Theme.cpp
)

set(CORE_HEADERS
    include/dstools/AppInit.h
    include/dstools/Config.h
    include/dstools/ErrorHandling.h
    include/dstools/ThreadUtils.h
    include/dstools/Theme.h
)

add_library(${PROJECT_NAME} STATIC ${CORE_SOURCES} ${CORE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Optional: qBreakpad support
find_package(qBreakpad CONFIG QUIET)
if(qBreakpad_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE qBreakpad::qBreakpad)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_QBREAKPAD)
endif()

# Optional: cpp-pinyin support
find_package(cpp-pinyin CONFIG QUIET)
if(cpp-pinyin_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE cpp-pinyin::cpp-pinyin)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_CPP_PINYIN)
endif()
