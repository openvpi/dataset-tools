project(minimal-appshell)

set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME}
    main.cpp
    HelloPage.h
    HelloPage.cpp
    CounterPage.h
    CounterPage.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dsfw-core
    dsfw-ui-core
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)
