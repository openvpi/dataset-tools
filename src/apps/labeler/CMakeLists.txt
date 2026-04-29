project(DiffSingerLabeler VERSION 0.1.0.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME}
    main.cpp
    LabelerWindow.cpp
    LabelerWindow.h
    StepNavigator.cpp
    StepNavigator.h
    IPageActions.h
    IPageLifecycle.h
    IPageProgress.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dstools-widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
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
