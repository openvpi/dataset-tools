project(minimal-task-processor)

set(CMAKE_AUTOMOC ON)

add_executable(${PROJECT_NAME}
    main.cpp
    WordCountProcessor.h
    WordCountProcessor.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Core
)
