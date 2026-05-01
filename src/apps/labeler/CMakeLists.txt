project(DiffSingerLabeler VERSION 0.1.0.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME}
    main.cpp
    CleanupDialog.cpp
    CleanupDialog.h
    TaskWindowAdapter.cpp
    TaskWindowAdapter.h
    pages/BuildCsvPage.cpp
    pages/BuildCsvPage.h
    pages/GameAlignPage.cpp
    pages/GameAlignPage.h
    pages/BuildDsPage.cpp
    pages/BuildDsPage.h
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dstools-widgets
    minlabel-page
    phonemelabeler-page
    pitchlabeler-page
    pipeline-pages
    game-infer
    rmvpe-infer
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(${PROJECT_NAME} PRIVATE .)

if(WIN32)
    include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)
    if(NOT ${CMAKE_BUILD_TYPE} MATCHES "Deb")
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${PROJECT_NAME})
